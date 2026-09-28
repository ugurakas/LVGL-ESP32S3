#include "wifi_manager.h"
#include <errno.h>
#include <string.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/dhcpv4.h>
#include <zephyr/sys/atomic.h>

LOG_MODULE_REGISTER(wifi_manager);
static struct net_if *iface;
static struct net_mgmt_event_callback wifi_cb, ip_cb;
static struct wifi_connect_req_params params;
static char station_ssid[33], station_password[65];
static atomic_t state;
static atomic_t associated;
static struct k_work_delayable connect_work;
static struct k_work_q wifi_queue;
K_THREAD_STACK_DEFINE(app_wifi_stack, 3072);
K_SEM_DEFINE(ready, 0, 1);

static void retry(void)
{
    atomic_set(&state, WIFI_MANAGER_DISCONNECTED);
    atomic_clear(&associated);
    k_sem_reset(&ready);
    k_work_reschedule_for_queue(&wifi_queue, &connect_work,
                               K_SECONDS(CONFIG_APP_WIFI_RETRY_SECONDS));
}

static void connect_handler(struct k_work *work)
{
    ARG_UNUSED(work);
    if (wifi_manager_is_connected()) {
        return;
    }
    /* Retry a timed-out association too; bound each request in the driver. */
    atomic_set(&state, WIFI_MANAGER_CONNECTING);
    int err = net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &params, sizeof(params));
    if (err && err != -EALREADY) {
        LOG_WRN("Connection request failed: %d", err);
        retry();
    }
}

static void wifi_event(struct net_mgmt_event_callback *cb,
                       uint32_t event, struct net_if *event_iface)
{
    if (event_iface != iface) {
        return;
    }
    if (event == NET_EVENT_WIFI_CONNECT_RESULT) {
        const struct wifi_status *result = cb->info;
        if (!result || result->status) {
            LOG_WRN("Wi-Fi association failed");
            retry();
            return;
        }
        atomic_set(&associated, 1);
        net_dhcpv4_start(iface);
    } else if (event == NET_EVENT_WIFI_DISCONNECT_RESULT) {
        net_dhcpv4_stop(iface);
        retry();
    }
}

static void ip_event(struct net_mgmt_event_callback *cb,
                     uint32_t event, struct net_if *event_iface)
{
    ARG_UNUSED(cb);
    if (event_iface != iface) {
        return;
    }
    if (event == NET_EVENT_IPV4_DHCP_BOUND && atomic_get(&associated)) {
        atomic_set(&state, WIFI_MANAGER_CONNECTED);
        k_sem_give(&ready);
        LOG_INF("Wi-Fi IPv4 ready");
    } else if (event == NET_EVENT_IPV4_ADDR_DEL) {
        atomic_set(&state, WIFI_MANAGER_CONNECTING);
        k_sem_reset(&ready);
    }
}

int wifi_manager_init(const char *ssid, const char *password)
{
    if (!ssid || !password || !strlen(ssid) || strlen(ssid) > 32 ||
        (strlen(password) && (strlen(password) < 8 || strlen(password) > 63))) {
        return -EINVAL;
    }
    iface = net_if_get_wifi_sta();
    if (!iface) {
        return -ENODEV;
    }
    strcpy(station_ssid, ssid);
    strcpy(station_password, password);
    params = (struct wifi_connect_req_params) {
        .ssid = (const uint8_t *)station_ssid,
        .ssid_length = strlen(station_ssid),
        .psk = (const uint8_t *)station_password,
        .psk_length = strlen(station_password),
        .channel = WIFI_CHANNEL_ANY,
        .security = strlen(station_password) ? WIFI_SECURITY_TYPE_PSK :
                                              WIFI_SECURITY_TYPE_NONE,
        .mfp = WIFI_MFP_OPTIONAL,
        .timeout = 20,
    };
    k_work_queue_start(&wifi_queue, app_wifi_stack, K_THREAD_STACK_SIZEOF(app_wifi_stack),
                       6, NULL);
    k_work_init_delayable(&connect_work, connect_handler);
    net_mgmt_init_event_callback(&wifi_cb, wifi_event,
        NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT);
    net_mgmt_add_event_callback(&wifi_cb);
    net_mgmt_init_event_callback(&ip_cb, ip_event,
        NET_EVENT_IPV4_DHCP_BOUND | NET_EVENT_IPV4_ADDR_DEL);
    net_mgmt_add_event_callback(&ip_cb);
    k_work_schedule_for_queue(&wifi_queue, &connect_work, K_NO_WAIT);
    return 0;
}

wifi_manager_state_t wifi_manager_get_state(void)
{
    return (wifi_manager_state_t)atomic_get(&state);
}

bool wifi_manager_is_connected(void)
{
    return wifi_manager_get_state() == WIFI_MANAGER_CONNECTED;
}

int wifi_manager_wait_ready(k_timeout_t timeout)
{
    if (wifi_manager_is_connected()) {
        return 0;
    }
    int err = k_sem_take(&ready, timeout);
    return err ? err : (wifi_manager_is_connected() ? 0 : -ENETDOWN);
}
