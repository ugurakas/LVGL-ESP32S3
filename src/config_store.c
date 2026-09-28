#include "config_store.h"
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/util.h>

static struct app_config config = { .api_path = "/", .api_body = "{}", .ntp_host = "pool.ntp.org" };
K_MUTEX_DEFINE(config_lock);

static char *field(const char *key, size_t *size)
{
    if (!key) {
        return NULL;
    }
    if (!strcmp(key, "ssid")) {
        *size = sizeof(config.ssid);
        return config.ssid;
    }
    if (!strcmp(key, "password")) {
        *size = sizeof(config.password);
        return config.password;
    }
    if (!strcmp(key, "api_host")) {
        *size = sizeof(config.api_host);
        return config.api_host;
    }
    if (!strcmp(key, "api_path")) {
        *size = sizeof(config.api_path);
        return config.api_path;
    }
    if (!strcmp(key, "api_body")) {
        *size = sizeof(config.api_body);
        return config.api_body;
    }
    if (!strcmp(key, "ntp_host")) {
        *size = sizeof(config.ntp_host);
        return config.ntp_host;
    }
    return NULL;
}

static int load_setting(const char *key, size_t len,
                        settings_read_cb read_cb, void *cb_arg)
{
    char value[1024] = {0};
    size_t capacity;
    char *dst = field(key, &capacity);
    if (!dst) {
        return -ENOENT;
    }
    if (len >= capacity) {
        return -EINVAL;
    }
    ssize_t read = read_cb(cb_arg, value, len);
    if (read < 0) {
        return (int)read;
    }
    if ((size_t)read != len || memchr(value, '\0', len)) {
        return -EINVAL;
    }
    k_mutex_lock(&config_lock, K_FOREVER);
    memset(dst, 0, capacity);
    memcpy(dst, value, len);
    k_mutex_unlock(&config_lock);
    return 0;
}
SETTINGS_STATIC_HANDLER_DEFINE(iot, "iot", NULL, load_setting, NULL, NULL);

int config_store_init(void)
{
    int err = settings_subsys_init();
    return err ? err : settings_load_subtree("iot");
}

void config_store_get(struct app_config *out)
{
    k_mutex_lock(&config_lock, K_FOREVER);
    *out = config;
    k_mutex_unlock(&config_lock);
}

int config_store_set(const char *key, const char *value)
{
    size_t capacity;
    char path[48];
    char *dst = field(key, &capacity);
    if (!dst || !value) {
        return -EINVAL;
    }
    size_t len = strlen(value);
    if (len >= capacity || (!strcmp(key, "api_path") && (!len || value[0] != '/'))) {
        return -EINVAL;
    }
    if (!strcmp(key, "password") && len && (len < 8 || len > 63)) {
        return -EINVAL;
    }
    if ((!strcmp(key, "api_host") || !strcmp(key, "ntp_host")) &&
        strpbrk(value, "\r\n/ :")) {
        return -EINVAL;
    }
    if (!strcmp(key, "api_path") && strpbrk(value, "\r\n")) {
        return -EINVAL;
    }
    /* Serialize flash and RAM updates so concurrent writers cannot diverge. */
    k_mutex_lock(&config_lock, K_FOREVER);
    snprintf(path, sizeof(path), "iot/%s", key);
    int err = settings_save_one(path, value, len);
    if (!err) {
        memset(dst, 0, capacity);
        memcpy(dst, value, len);
    }
    k_mutex_unlock(&config_lock);
    return err;
}
