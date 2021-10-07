#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi_types.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "WifiWrap.h"

#define DEFAULT_SCAN_METHOD WIFI_ALL_CHANNEL_SCAN
#define DEFAULT_SORT_METHOD WIFI_CONNECT_AP_BY_SECURITY
#define DEFAULT_RSSI 1
#define DEFAULT_AUTHMODE WIFI_AUTH_WPA2_PSK
#define EXAMPLE_ESP_MAXIMUM_RETRY  40

static const char *TAG = "wifi station";

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

WifiWrap::WifiWrap() 
{
}

void WifiWrap::connect() 
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    // Initialize default station as network interface instance (esp-netif)
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    // Initialize and start WiFi

    uint8_t default_ssid[32] = { 1,2,3,4,5,6,7,8 };
    uint8_t default_pwd[64] = { 1, 2, 3, 4 };
    wifi_config_t wifi_config;
    memcpy(wifi_config.sta.ssid, default_ssid, 32);
    memcpy(wifi_config.sta.password, default_pwd, 64);
    wifi_config.sta.scan_method = DEFAULT_SCAN_METHOD;
    wifi_config.sta.sort_method = DEFAULT_SORT_METHOD;
    wifi_config.sta.threshold = {
        DEFAULT_RSSI,
        DEFAULT_AUTHMODE
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

esp_err_t WifiWrap::disconnect() 
{
    esp_err_t status = ESP_OK;
    status = esp_wifi_deinit();

    return status;
}


void WifiWrap::wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data) 
{
    static int s_retry_num = 0;

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