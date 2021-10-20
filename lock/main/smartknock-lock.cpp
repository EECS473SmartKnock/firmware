#include <stdio.h>

#include "SmartKnockAPI.h"
#include "WifiWrap.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "nvs_flash.h"

extern "C" {
void app_main();
}

void app_main() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    WifiWrap wifiwrapper;
    char network_name[] = "gogoinflight";
    char password[] = "mitrealityhack";
    WifiPassHeader header{network_name, strlen(network_name), password, strlen(password)};

    wifiwrapper.connect(header);

    SmartKnockAPI api;
    for (;;) {
        MessageType m = api.get_incoming_message();
        if (m == MessageType::LOCK) {
            ESP_LOGI("SmartKnock", "LOCK message received\n");
        }
        if (m == MessageType::UNLOCK) {
            ESP_LOGI("SmartKnock", "UNLOCK message received\n");
        }
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}
