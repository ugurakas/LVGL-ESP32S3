#include "ui.h"
#include "sensor_model.h"
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#ifdef CONFIG_APP_NETWORK
#include "config_store.h"
#include "wifi_manager.h"
#include "api_client.h"
#endif
LOG_MODULE_REGISTER(panel_app);
int main(void)
{
    const struct device *display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
    if (!device_is_ready(display)) {
        LOG_ERR("Display device is not ready");
        return 0;
    }
    struct display_capabilities caps;
    display_get_capabilities(display, &caps);
    if (caps.x_resolution != 128 || caps.y_resolution != 128) {
        LOG_ERR("This UI requires a 128x128 display");
        return 0;
    }
    ui_init();
    lv_timer_handler();
    display_blanking_off(display);
#ifdef CONFIG_APP_NETWORK
    static struct app_config config;
    int err = config_store_init();
    if (!err) {
        config_store_get(&config);
        if (config.ssid[0]) {
            err = wifi_manager_init(config.ssid, config.password);
            if (!err && config.api_host[0]) {
                err = api_client_start(&config);
            }
        } else {
            LOG_INF("Provision with 'panel set', then reboot");
        }
    }
    if (err) {
        sensor_model_error(err);
        LOG_ERR("Network initialization failed: %d", err);
    }
#endif
    /* Only this thread calls LVGL. Workers publish plain data under a mutex. */
    for (;;) {
        struct sensor_state state;
        sensor_model_get(&state);
        bool online = false;
#ifdef CONFIG_APP_NETWORK
        online = wifi_manager_is_connected();
#endif
        ui_render(&state, online);
        lv_timer_handler();
        k_sleep(K_MSEC(20));
    }
}
