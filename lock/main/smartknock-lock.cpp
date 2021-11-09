#include <stdio.h>

// Internal Libraries
#include "SmartKnockAPI.h"
#include "WifiWrap.h"
#include "DeepSleep.h"
#include "ulp_adc.h"

// External Libraries
#include "freertos/semphr.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

extern "C" {
void app_main();
}

// Global Defines Variables
#define GLOBAL_STACK_SIZE   8000
#define GLOBAL_CPU_CORE_0   0
#define GLOBAL_CPU_CORE_1   1

SemaphoreHandle_t task_wifi_complete;

ULP ulp;

// Global Defines Tasks

void task_scan_handler(void * pvParameters);

void task_sleep_handler(void * pvParameters);

void app_main() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /*  Initialize API Libraries    */
    static SmartKnockAPI api;
    static DeepSleep sleep_wrapper;

    /*  Initialize Wifi */
    WifiWrap wifi_wrapper;
    char network_name[] = "ArthurZhang";
    char password[] = "arthurthesoccerball";
    WifiPassHeader header{network_name, strlen(network_name), password, strlen(password)};

    wifi_wrapper.connect(header);

    /*  Initialize Deep Sleep Handlers  */
    SleepConfig sleep_config = {
        &wifi_wrapper,
        GPIO_NUM_2,
        1,
        10000000 // in us,
    };
    sleep_wrapper.config(sleep_config);
    sleep_wrapper.print_wakeup_reason();

    ULPConfig ulp_config = {
        GPIO_NUM_27, 
        GPIO_NUM_13
    };
    ulp.enable_ulp_monitoring(ulp_config);

    /*  Start Tasks */
    BaseType_t xTaskAPIReturned;
    BaseType_t xTaskSleep;
    TaskHandle_t xAPIHandle     = NULL;
    TaskHandle_t xSleepHandle   = NULL;

    task_wifi_complete = xSemaphoreCreateBinary();

    xTaskAPIReturned = xTaskCreatePinnedToCore(
                            task_scan_handler,
                            "APIScan",
                            GLOBAL_STACK_SIZE,
                            &api,
                            1,
                            &xAPIHandle,
                            GLOBAL_CPU_CORE_0);
    xTaskSleep = xTaskCreatePinnedToCore(
                            task_sleep_handler,
                            "SleepHandler",
                            GLOBAL_STACK_SIZE,
                            &sleep_wrapper,
                            0,
                            &xSleepHandle,
                            GLOBAL_CPU_CORE_0);
}

void task_sleep_handler(void * pvParameters)
{
    configASSERT( static_cast<DeepSleep*>(pvParameters) != nullptr);

    DeepSleep* sleep_handler = static_cast<DeepSleep*>(pvParameters);

    while (xSemaphoreTake(task_wifi_complete, ( TickType_t ) 0) != pdTRUE) {}
    // Begins sleep mode after all processes are finished
    ESP_LOGI("SmartKnock", " Beginning sleep mode\n");
    ulp.start_ulp_monitoring();
    sleep_handler->sleep();
    for (;;)
    {}
}

void task_scan_handler(void * pvParameters)
{
    configASSERT( static_cast<SmartKnockAPI*>(pvParameters) != nullptr);

    TickType_t xLastWakeTime;
    SmartKnockAPI* api_handle = static_cast<SmartKnockAPI*>(pvParameters);
    xSemaphoreTake(task_wifi_complete, ( TickType_t ) 0);
    for (;;) 
    {

        xLastWakeTime = xTaskGetTickCount();
        MessageType m = api_handle->get_incoming_message();
        if (m == MessageType::LOCK) {
            ESP_LOGI("SmartKnock", "LOCK message received\n");
            xSemaphoreGive(task_wifi_complete);
        }
        if (m == MessageType::UNLOCK) {
            ESP_LOGI("SmartKnock", "UNLOCK message received\n");
            xSemaphoreGive(task_wifi_complete);
        }
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(30000)); // Scan every 30 s
    }
}