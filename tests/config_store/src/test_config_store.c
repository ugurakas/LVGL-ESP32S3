#include "config_store.h"
#include <errno.h>
#include <string.h>
#include <zephyr/settings/settings.h>
#include <zephyr/ztest.h>

static int save_error;
static char saved_name[48], saved_value[128];
static size_t saved_len;

int __wrap_settings_save_one(const char *name, const void *value, size_t len)
{
    if (save_error) {
        return save_error;
    }
    strcpy(saved_name, name);
    memset(saved_value, 0, sizeof(saved_value));
    memcpy(saved_value, value, len);
    saved_len = len;
    return 0;
}

ZTEST(config_store, test_invalid_values_do_not_mutate)
{
    struct app_config before, after;
    char oversized[130];
    memset(oversized, 'x', sizeof(oversized) - 1);
    oversized[sizeof(oversized) - 1] = 0;
    config_store_get(&before);
    zassert_equal(config_store_set("unknown", "value"), -EINVAL);
    zassert_equal(config_store_set("ssid", oversized), -EINVAL);
    zassert_equal(config_store_set("password", "short"), -EINVAL);
    zassert_equal(config_store_set("api_path", ""), -EINVAL);
    zassert_equal(config_store_set("api_path", "no-leading-slash"), -EINVAL);
    zassert_equal(config_store_set("api_host", "https://invalid"), -EINVAL);
    zassert_equal(config_store_set("api_host", "host\r\nInjected"), -EINVAL);
    zassert_equal(config_store_set("api_path", "/path\r\nInjected"), -EINVAL);
    config_store_get(&after);
    zassert_mem_equal(&before, &after, sizeof(before));
}

ZTEST(config_store, test_save_failure_preserves_ram)
{
    struct app_config before, after;
    config_store_get(&before);
    save_error = -EIO;
    zassert_equal(config_store_set("ssid", "test-network"), -EIO);
    save_error = 0;
    config_store_get(&after);
    zassert_mem_equal(&before, &after, sizeof(before));
}

ZTEST(config_store, test_save_and_load)
{
    struct app_config config;
    zassert_ok(config_store_set("ssid", "test-network"));
    zassert_equal(strcmp(saved_name, "iot/ssid"), 0);
    zassert_equal(saved_len, strlen("test-network"));
    zassert_equal(strcmp(saved_value, "test-network"), 0);
    zassert_ok(settings_runtime_set("iot/ssid", "restored", strlen("restored")));
    config_store_get(&config);
    zassert_equal(strcmp(config.ssid, "restored"), 0);

    char malformed[] = {'a', 0, 'b'};
    zassert_equal(settings_runtime_set("iot/ssid", malformed, sizeof(malformed)),
                  -EINVAL);
    config_store_get(&config);
    zassert_equal(strcmp(config.ssid, "restored"), 0);
}

static void *setup(void)
{
    zassert_ok(config_store_init());
    return NULL;
}
ZTEST_SUITE(config_store, NULL, setup, NULL, NULL, NULL);
