#ifndef SENSOR_UI_H
#define SENSOR_UI_H
#include <lvgl.h>
#include "sensor_model.h"
void ui_init(void);
void ui_render(const struct sensor_state *state, bool online);
lv_obj_t *ui_value_label(void);
lv_obj_t *ui_refresh_button(void);
#endif
