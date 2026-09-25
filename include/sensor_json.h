#ifndef SENSOR_JSON_H
#define SENSOR_JSON_H
#include <stddef.h>
#include <stdint.h>
/* Length excludes the terminating NUL; input must have length + 1 bytes. */
int sensor_json_parse(const char *json, size_t length, int32_t *centi_value);
#endif
