#include "ui.h"
#include <stdio.h>
#include <zephyr/kernel.h>
static lv_obj_t *value_label, *status_label, *age_label, *refresh_button;
static void refresh(lv_event_t *event)
{
    ARG_UNUSED(event);
    sensor_request_refresh();
}
void ui_init(void)
{
    lv_obj_t *screen = lv_screen_active();
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x171c28), 0);
    lv_obj_set_style_text_color(screen, lv_color_hex(0xf1f4fa), 0);

    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "SENSOR");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);

    value_label = lv_label_create(screen);
    lv_obj_set_style_text_font(value_label, &lv_font_montserrat_24, 0);
    lv_obj_set_width(value_label, 120);
    lv_obj_set_style_text_align(value_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(value_label, LV_LABEL_LONG_DOT);
    lv_label_set_text(value_label, "--.--");
    lv_obj_align(value_label, LV_ALIGN_TOP_MID, 0, 27);

    status_label = lv_label_create(screen);
    lv_label_set_text(status_label, "Wi-Fi offline");
    lv_obj_align(status_label, LV_ALIGN_TOP_MID, 0, 58);

    age_label = lv_label_create(screen);
    lv_label_set_text(age_label, "No data");
    lv_obj_align(age_label, LV_ALIGN_TOP_MID, 0, 77);

    refresh_button = lv_button_create(screen);
    lv_obj_set_size(refresh_button, 112, 25);
    lv_obj_align(refresh_button, LV_ALIGN_BOTTOM_MID, 0, -3);
    lv_obj_set_style_bg_color(refresh_button, lv_color_hex(0x8b323f), 0);
    lv_obj_add_event_cb(refresh_button, refresh, LV_EVENT_CLICKED, NULL);
    lv_obj_t *label = lv_label_create(refresh_button);
    lv_label_set_text(label, "Refresh");
    lv_obj_center(label);
}
void ui_render(const struct sensor_state *state, bool online)
{
    char value[32];
    char age[32];
    if (state->valid) {
        int64_t signed_value = state->centi_value;
        uint32_t magnitude = signed_value < 0 ? -signed_value : signed_value;
        snprintf(value, sizeof(value), "%s%u.%02u",
                 signed_value < 0 ? "-" : "", magnitude / 100, magnitude % 100);
        lv_label_set_text(value_label, value);
        int64_t seconds = MAX(0, (k_uptime_get() - state->updated_ms) / 1000);
        snprintf(age, sizeof(age), "%llds ago", (long long)seconds);
        lv_label_set_text(age_label, age);
    } else {
        lv_label_set_text(value_label, "--.--");
        lv_label_set_text(age_label, "No data");
    }
    lv_label_set_text(status_label, !online ? "Wi-Fi offline" :
                      state->error ? "API error" : "Wi-Fi online");
    lv_obj_align(value_label, LV_ALIGN_TOP_MID, 0, 27);
    lv_obj_align(status_label, LV_ALIGN_TOP_MID, 0, 58);
    lv_obj_align(age_label, LV_ALIGN_TOP_MID, 0, 77);
}
lv_obj_t *ui_value_label(void) { return value_label; }
lv_obj_t *ui_refresh_button(void) { return refresh_button; }
