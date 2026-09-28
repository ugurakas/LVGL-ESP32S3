#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H
#include <stdbool.h>
#include <zephyr/kernel.h>

typedef enum {
    WIFI_MANAGER_DISCONNECTED,
    WIFI_MANAGER_CONNECTING,
    WIFI_MANAGER_CONNECTED
} wifi_manager_state_t;
/* Initialize once from main after settings have loaded. */
int wifi_manager_init(const char *ssid, const char *password);
bool wifi_manager_is_connected(void);
wifi_manager_state_t wifi_manager_get_state(void);
/* Single MQTT consumer; always rechecks current state after waking. */
int wifi_manager_wait_ready(k_timeout_t timeout);
#endif
