#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lv_port.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "freertos/semphr.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "ui.h"
#include "ui_helpers.h"
#include "lvgl.h"
//#include "bsp_lcd.h"
//#include "bsp_board.h"

//////*Htttp post ile ilgili*/
#include "esp_http_client.h"
#include "http_parser.h"
#include "esp_sntp.h"
#include "esp_netif.h"

#include "cJSON.h"


#include "lwip/dns.h"

#include "esp_tls.h"
#include "sdkconfig.h"
//#include "projdefs.h"
#include "esp_heap_caps.h"
#include "esp_err.h"
#include "esp_timer.h"


/* The examples use WiFi configuration that you can set via project configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
static TaskHandle_t lvgl_task_handle;

#define LV_PORT_TASK_DELAY_MS           (1000)

#define EXAMPLE_ESP_WIFI_SSID      ID
#define EXAMPLE_ESP_WIFI_PASS      PASS
#define EXAMPLE_ESP_MAXIMUM_RETRY  5


#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif

void ui_Screen1_screen_init(void);
extern lv_obj_t *ui_Screen1;
extern lv_obj_t *ui_sensornameTemp;
extern lv_obj_t *ui_Temperature;
void ui_event_Templabel( lv_event_t * e);
extern lv_obj_t *ui_Templabel;
void ui_event_namelabel( lv_event_t * e);
extern lv_obj_t *ui_namelabel;
void ui_event_Time( lv_event_t * e);
extern lv_obj_t *ui_Time;
void ui_event_Day( lv_event_t * e);
extern lv_obj_t *ui_Day;
extern lv_obj_t *ui____initial_actions0;
int roundedValue;
int secondValue;
volatile char value_str[32];
//double value;
/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = "Ugur's propert";

static int s_retry_num = 0;

void process_json_data(const char *json_data) {
    cJSON *root = cJSON_Parse(json_data);
    if (root == NULL) {
        // JSON ayrıştırma hatası
        return;
    }

    cJSON *things = cJSON_GetObjectItem(root, "things");
    if (things != NULL && cJSON_IsArray(things)) {
        // things dizisinin ilk öğesini al
        cJSON *thing = cJSON_GetArrayItem(things, 0);

        if (thing != NULL) {
            // thing içerisindeki "device" öğesini al
            cJSON *device = cJSON_GetObjectItem(thing, "device");

            if (device != NULL) {
                // device içerisindeki "SensorValue" öğesini al
                cJSON *sensorValue = cJSON_GetObjectItem(device, "SensorValue");

                if (sensorValue != NULL && cJSON_IsArray(sensorValue)) {
                    // SensorValue dizisinin ilk öğesini al
                    cJSON *firstValue = cJSON_GetArrayItem(sensorValue, 0);

                    if (firstValue != NULL) {
                        // "value" öğesini al
                        cJSON *value = cJSON_GetObjectItem(firstValue, "value");

                        if (value != NULL && cJSON_IsNumber(value)) { 
                            //secondValue 
                            const double value_double = value->valuedouble;
                            snprintf(value_str, sizeof(value_str), "%.2f", value_double);
                            printf("String Value%s:\n ",value_str);
                            /*
                            roundedValue = round(value->valuedouble);
                            printf("Rounded Value:%d:\n",roundedValue);
                            
                            printf("label_set_öncesi\n");
                            
                            printf("label_set_sonrası");
                            */
                           lv_label_set_text(ui_Templabel,value_str);
                        } else {
                            printf("\"value\" alanı bir sayı değil.\n");
                        }
                    }
                }
            }
        }
    }
    

    cJSON_Delete(root);
}
/**************************          lvgl_label_reflesh Request          ************************/

esp_err_t client_event_post_handler(esp_http_client_event_handle_t evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ON_DATA:
        
        printf("Client HTTP_EVENT_ON_DATA: %.*s\n", evt->data_len, (char *)evt->data);

        printf("SizeOfData: %ld\n", strlen(evt->data));
        
        process_json_data(evt->data);
        

        break;
    default:
        break;
    }
    return ESP_OK;
}
/**************************          Http Request          ************************/
static void client_post_rest_function()
{
    esp_http_client_config_t config_post = {
        .url = "https://api.dashboard.sade.io/Thing/GetThingsWithDevice",
        .method = HTTP_METHOD_POST,
        .cert_pem = NULL,
        .auth_type = HTTP_AUTH_TYPE_BASIC,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .timeout_ms = 10000,
        //.crt_bundle_attach = esp_crt_bundle_attach,
        .event_handler = client_event_post_handler
        };
        
    esp_http_client_handle_t client = esp_http_client_init(&config_post);

    const char  *data = "{\"memberId\": 1987,\"authTokenSelector\": \"Foz2ExTIRtzN\",\"authToken\": \"f83b3808577ce4f1b41ff9f92d1a43b8da967e59fcc515d3fb0dce897e72a10b\",\"clientId\": 387,\"applicationId\": 118}";
    esp_http_client_set_post_field(client, data, strlen(data));
    esp_http_client_set_header(client, "Content-Type", "application/json");
    ESP_LOGI(TAG, "Response");
    
    //cJSON *root = cJSON_Parse(client);

    //char *p_json = strstr(client, "\r\n\r\n");
    
    //__http_data_prase(client->parse);
    
    //ESP_LOGI(TAG, "Response1:");
    esp_http_client_perform(client);
    vTaskDelay(500);
    esp_http_client_cleanup(client);
}


static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));



    wifi_config_t wifi_config = {
        .sta = {
            .ssid = ID,
            .password = PASS,
            /* Authmode threshold resets to WPA2 as default if password matches WPA2 standards (pasword len => 8).
             * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
             * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
	     * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
             */
            //.threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
        },      
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );
    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
                 //printf("Client HTTP_EVENT_ON_DATA:\n");
                 //client_post_rest_function();
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
}
void http_request_task(void *pvParameters)
{
    
        // Perform HTTP POST request
        client_post_rest_function();

        // Delay for 10 seconds before the next request
        vTaskDelay(LV_PORT_TASK_DELAY_MS);
        
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    wifi_init_sta();
    
    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(bsp_board_init());
    lv_port_init();
    lv_port_sem_take();
    ui_init();
    lv_port_sem_give();
    
    xTaskCreate(http_request_task, "client_post", 4096, NULL, 4, NULL);
    //xTaskCreate(label_refresher_task,"label_refresher_task",4096, NULL, 5, NULL);
    //int x=1;
    //xTaskCreate(client_post_rest_function, "lvgl_task",4096 * 4, NULL, 1, &lvgl_task_handle);
    /*for (;;) {

        client_post_rest_function();
        //vTaskDelay(LV_PORT_TASK_DELAY_MS);
        lv_label_set_text(ui_Templabel,value_str);
    
    }*/
    }
