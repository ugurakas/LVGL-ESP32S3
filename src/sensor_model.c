#include "sensor_model.h"
#include <zephyr/kernel.h>
static struct sensor_state state;
K_MUTEX_DEFINE(sensor_lock);
K_SEM_DEFINE(refresh_signal, 0, 1);
void sensor_model_update(int32_t value)
{
    k_mutex_lock(&sensor_lock, K_FOREVER);
    state = (struct sensor_state) {
        .valid = true, .centi_value = value, .updated_ms = k_uptime_get()
    };
    k_mutex_unlock(&sensor_lock);
}
void sensor_model_error(int error)
{
    k_mutex_lock(&sensor_lock, K_FOREVER);
    state.error = error;
    k_mutex_unlock(&sensor_lock);
}
void sensor_model_get(struct sensor_state *out)
{
    k_mutex_lock(&sensor_lock, K_FOREVER);
    *out = state;
    k_mutex_unlock(&sensor_lock);
}
void sensor_request_refresh(void)
{
    k_sem_give(&refresh_signal);
}
int sensor_wait_refresh(k_timeout_t timeout)
{
    return k_sem_take(&refresh_signal, timeout);
}
