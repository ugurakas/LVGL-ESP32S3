#include "sensor_json.h"
#include <errno.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <cJSON.h>

int sensor_json_parse(const char *json, size_t length, int32_t *centi_value)
{
    if (!json || !centi_value || !length || length > 4095 ||
        json[length] != '\0' || memchr(json, '\0', length)) {
        return -EINVAL;
    }
    cJSON *root = cJSON_ParseWithLengthOpts(json, length + 1, NULL, true);
    if (!root) {
        return -EBADMSG;
    }
    const cJSON *things = cJSON_GetObjectItemCaseSensitive(root, "things");
    const cJSON *thing = cJSON_GetArrayItem(things, 0);
    const cJSON *device = cJSON_GetObjectItemCaseSensitive(thing, "device");
    const cJSON *values = cJSON_GetObjectItemCaseSensitive(device, "SensorValue");
    const cJSON *sample = cJSON_GetArrayItem(values, 0);
    const cJSON *value = cJSON_GetObjectItemCaseSensitive(sample, "value");
    int err = -EBADMSG;
    if (cJSON_IsArray(things) && cJSON_IsArray(values) && cJSON_IsNumber(value) &&
        isfinite(value->valuedouble) && fabs(value->valuedouble) <= 100000.0) {
        double scaled = value->valuedouble * 100.0;
        *centi_value = (int32_t)(scaled + (scaled < 0 ? -0.5 : 0.5));
        err = 0;
    }
    cJSON_Delete(root);
    return err;
}
