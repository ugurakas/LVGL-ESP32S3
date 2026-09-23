#include "api_client.h"
#include "wifi_manager.h"
#include "sensor_model.h"
#include "sensor_json.h"
#include "response_buffer.h"
#include <errno.h>
#include <string.h>
#include <time.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/http/client.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/sntp.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/posix/time.h>
LOG_MODULE_REGISTER(api_client);
static struct app_config config;
static const unsigned char ca_certificate[] = {
#include "api_ca.inc"
};
static const sec_tag_t ca_tag = 1;
static struct k_thread api_thread;
K_THREAD_STACK_DEFINE(panel_api_stack, 8192);
static uint8_t receive_buffer[1024];
static struct response_buffer response;
static bool complete;
static int http_status;

static void receive_response(struct http_response *rsp,
                             enum http_final_call final, void *user_data)
{
    ARG_UNUSED(user_data);
    http_status = rsp->http_status_code;
    response_buffer_append(&response, rsp->body_frag_start, rsp->body_frag_len);
    if (final == HTTP_DATA_FINAL) {
        complete = true;
    }
}

static int synchronize_time(void)
{
    struct sntp_time time;
    int err = sntp_simple(config.ntp_host, 10000, &time);
    if (err) {
        return err;
    }
    struct timespec now = { .tv_sec = time.seconds, .tv_nsec = 0 };
    return clock_settime(CLOCK_REALTIME, &now) ? -errno : 0;
}

static int request_sample(void)
{
    struct zsock_addrinfo *addresses = NULL;
    struct zsock_addrinfo hints = {
        .ai_family = AF_INET, .ai_socktype = SOCK_STREAM
    };
    if (zsock_getaddrinfo(config.api_host, "443", &hints, &addresses)) {
        return -EHOSTUNREACH;
    }
    int socket = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TLS_1_2);
    int err = 0;
    if (socket < 0) {
        err = -errno;
        goto free_addresses;
    }
    int verify = TLS_PEER_VERIFY_REQUIRED;
    struct timeval timeout = { .tv_sec = 10 };
    if (zsock_setsockopt(socket, SOL_TLS, TLS_SEC_TAG_LIST, &ca_tag, sizeof(ca_tag)) ||
        zsock_setsockopt(socket, SOL_TLS, TLS_HOSTNAME, config.api_host,
                         strlen(config.api_host)) ||
        zsock_setsockopt(socket, SOL_TLS, TLS_PEER_VERIFY, &verify, sizeof(verify)) ||
        zsock_setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) ||
        zsock_setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout))) {
        err = -errno;
        goto close_socket;
    }
    if (!addresses || zsock_connect(socket, addresses->ai_addr, addresses->ai_addrlen)) {
        err = addresses ? -errno : -EHOSTUNREACH;
        goto close_socket;
    }
    response = (struct response_buffer){0};
    complete = false;
    http_status = 0;
    struct http_request request = {
        .method = HTTP_POST,
        .url = config.api_path,
        .host = config.api_host,
        .protocol = "HTTP/1.1",
        .content_type_value = "application/json",
        .payload = config.api_body,
        .payload_len = strlen(config.api_body),
        .response = receive_response,
        .recv_buf = receive_buffer,
        .recv_buf_len = sizeof(receive_buffer),
    };
    err = http_client_req(socket, &request, 10000, NULL);
    if (err >= 0) {
        err = response.error ? response.error :
              !complete ? -EBADMSG :
              http_status != 200 ? -EACCES : 0;
        if (!err) {
            int32_t value;
            err = sensor_json_parse(response.data, response.used, &value);
            if (!err) {
                sensor_model_update(value);
            }
        }
    }
close_socket:
    zsock_close(socket);
free_addresses:
    if (addresses) {
        zsock_freeaddrinfo(addresses);
    }
    return err;
}

static void poll_api(void *a, void *b, void *c)
{
    ARG_UNUSED(a); ARG_UNUSED(b); ARG_UNUSED(c);
    bool time_ready = false;
    for (;;) {
        if (wifi_manager_wait_ready(K_SECONDS(5))) {
            continue;
        }
        int err = 0;
        if (!time_ready) {
            err = synchronize_time();
            time_ready = !err;
        }
        if (!err) {
            err = request_sample();
        }
        if (err) {
            sensor_model_error(err);
            LOG_WRN("Sensor request failed: %d", err);
        }
        sensor_wait_refresh(K_SECONDS(CONFIG_APP_POLL_SECONDS));
    }
}

int api_client_start(const struct app_config *cfg)
{
    if (!cfg || !cfg->api_host[0] || cfg->api_path[0] != '/') {
        return -EINVAL;
    }
    /* No verification bypass: an absent CA keeps HTTPS disabled. */
    if (sizeof(ca_certificate) <= 1) {
        return -ENOKEY;
    }
    int err = tls_credential_add(ca_tag, TLS_CREDENTIAL_CA_CERTIFICATE,
                                 ca_certificate, sizeof(ca_certificate));
    if (err) {
        return err;
    }
    config = *cfg;
    k_thread_create(&api_thread, panel_api_stack, K_THREAD_STACK_SIZEOF(panel_api_stack),
                    poll_api, NULL, NULL, NULL, 7, 0, K_NO_WAIT);
    return 0;
}
