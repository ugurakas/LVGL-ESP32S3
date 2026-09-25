#ifndef SENSOR_MODEL_H
#define SENSOR_MODEL_H
#include <stdbool.h>
#include <stdint.h>
#include <zephyr/kernel.h>
struct sensor_state {
    bool valid;
    int32_t centi_value;
    int64_t updated_ms;
    int error;
};
void sensor_model_update(int32_t value);
void sensor_model_error(int error);
void sensor_model_get(struct sensor_state *out);
void sensor_request_refresh(void);
int sensor_wait_refresh(k_timeout_t timeout);
#endif
