#include <stdio.h>
#include "nvs_flash.h"
#include "WifiWrap.h"
#include "SmartKnockAPI.h"
#include "freertos/FreeRTOS.h"

extern "C" {
    void app_main();
}

void app_main()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    WifiWrap wifiwrapper;
    char network_name[] = "LigmaSugma";
    char password[] = "VirginHostSheet420";
    WifiPassHeader header{  network_name, strlen(network_name),
                            password, strlen(password)};
 
    wifiwrapper.connect(header);

    SmartKnockAPI api;
    api.send_message(LockMessage());
    for(;;) 
    {
        // Block for 10ms to give idle task handler enough time to reset watchdog
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
