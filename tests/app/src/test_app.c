#include "sensor_json.h"
#include "sensor_model.h"
#include "response_buffer.h"
#include "ui.h"
#include <errno.h>
#include <string.h>
#include <zephyr/drivers/display.h>
#include <zephyr/ztest.h>

static const char valid_json[] =
    "{\"things\":[{\"device\":{\"SensorValue\":[{\"value\":-12.34}]}}]}";

ZTEST(panel, test_fragmented_response)
{
    struct response_buffer buffer = {0};
    size_t len = strlen(valid_json);
    for (size_t i = 0; i < len; ++i) {
        zassert_ok(response_buffer_append(&buffer, valid_json + i, 1));
    }
    int32_t value = 0;
    zassert_ok(sensor_json_parse(buffer.data, buffer.used, &value));
    zassert_equal(value, -1234);
}

ZTEST(panel, test_overflow_is_sticky)
{
    struct response_buffer buffer = {0};
    static char data[4096];
    memset(data, 'x', sizeof(data));
    zassert_equal(response_buffer_append(&buffer, data, sizeof(data)), -EMSGSIZE);
    zassert_equal(response_buffer_append(&buffer, "{}", 2), -EMSGSIZE);
    zassert_equal(buffer.used, 0);
}

ZTEST(panel, test_invalid_json_preserves_value)
{
    const char *invalid[] = {
        "{", "{}", "{\"things\":[]}",
        "{\"things\":[{\"device\":{\"SensorValue\":[{\"value\":\"12\"}]}}]}",
        "{\"things\":[{\"device\":{\"SensorValue\":[{\"value\":1e100}]}}]}",
        "{\"things\":[]}garbage"
    };
    for (size_t i = 0; i < ARRAY_SIZE(invalid); ++i) {
        int32_t value = 42;
        zassert_true(sensor_json_parse(invalid[i], strlen(invalid[i]), &value) < 0);
        zassert_equal(value, 42);
    }
}

ZTEST(panel, test_error_keeps_last_sample)
{
    struct sensor_state state;
    sensor_model_update(2510);
    sensor_model_error(-ETIMEDOUT);
    sensor_model_get(&state);
    zassert_true(state.valid);
    zassert_equal(state.centi_value, 2510);
    zassert_equal(state.error, -ETIMEDOUT);
    sensor_model_update(2520);
    sensor_model_get(&state);
    zassert_equal(state.error, 0);
}

ZTEST(panel, test_ui_128_and_refresh)
{
    struct sensor_state state = {
        .valid = true, .centi_value = -1234, .updated_ms = k_uptime_get()
    };
    ui_render(&state, true);
    lv_obj_update_layout(lv_screen_active());
    zassert_equal(lv_obj_get_width(lv_screen_active()), 128);
    zassert_equal(lv_obj_get_height(lv_screen_active()), 128);
    zassert_equal(strcmp(lv_label_get_text(ui_value_label()), "-12.34"), 0);
    lv_obj_t *button = ui_refresh_button();
    zassert_true(lv_obj_get_x(button) >= 0);
    zassert_true(lv_obj_get_y(button) >= 0);
    zassert_true(lv_obj_get_x(button) + lv_obj_get_width(button) <= 128);
    zassert_true(lv_obj_get_y(button) + lv_obj_get_height(button) <= 128);
    lv_obj_send_event(button, LV_EVENT_CLICKED, NULL);
    zassert_ok(sensor_wait_refresh(K_NO_WAIT));
    lv_timer_handler();
}

static void *setup(void)
{
    const struct device *display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
    zassert_true(device_is_ready(display));
    ui_init();
    return NULL;
}
ZTEST_SUITE(panel, NULL, setup, NULL, NULL, NULL);
