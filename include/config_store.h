#ifndef PANEL_CONFIG_STORE_H
#define PANEL_CONFIG_STORE_H
struct app_config {
    char ssid[33];
    char password[65];
    char api_host[128];
    char api_path[192];
    char api_body[1024];
    char ntp_host[128];
};
int config_store_init(void);
void config_store_get(struct app_config *out);
int config_store_set(const char *key, const char *value);
#endif
