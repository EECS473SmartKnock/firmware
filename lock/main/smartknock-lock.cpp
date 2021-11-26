#include <stdio.h>

// Internal Libraries
#include "BLE.h"
#include "DeepSleep.h"
#include "NVSWrapper.h"
#include "NimBLEDevice.h"
#include "SmartKnockAPI.h"
#include "WifiWrap.h"
#include "ulp_adc.h"

// External Libraries
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "nvs_flash.h"

extern "C" {
void app_main();
}

// Global Defines Variables
#define GLOBAL_STACK_SIZE 8000
#define GLOBAL_CPU_CORE_0 0
#define GLOBAL_CPU_CORE_1 1

SemaphoreHandle_t task_wifi_complete;

ULP ulp;
NVSWrapper nvs;
BLE ble;
// std::string passphrase = "";  // TODO: this should be set by the setup process and
                              // saved in non-volatile storage
// Global Defines Tasks

void task_scan_handler(void* pvParameters);

void task_sleep_handler(void* pvParameters);

void app_main() {
    /*  Initialize API Libraries    */
    static SmartKnockAPI api;
    static DeepSleep sleep_wrapper;

    nvs.init();

    /*

    Check if passphrase is set in NVS
        If not, begin setup process. Setup process consists of advertising on BLE and
    waiting for the user to submit the passphrase from the app.

        During setup process, no passphrase hash is required from the app

        If so, continue with normal operation. Now, to change WiFi config, the user must
    submit a hash(ssid+password+passphrase) from the app.

    */
    bool is_setup = nvs.existsStr("passphrase");
    if (is_setup) {
        std::string passphrase = nvs.get("passphrase");
        ESP_LOGI("SmartKnock", "Lock is set up and ready to go! Passphrase: %s", passphrase.c_str());
    } else {
        ESP_LOGI("SmartKnock", "Lock is not set up. Starting setup process...");
    }

    ble.init();

    ble.onSetupComplete = [&](const std::string& passphrase, const std::string& ssid,
                              const std::string& password) {
        if (!is_setup) {
            ESP_LOGI("BLE", "Setting up WiFi");
            nvs.set("ssid", ssid);
            nvs.set("password", password);
            nvs.set("passphrase", passphrase);
            nvs.commit();
        }

        // For now, just log it
        ESP_LOGI("SmartKnock", "Setup complete: %s, %s, %s", passphrase.c_str(),
                 ssid.c_str(), password.c_str());

        nvs.close();

        // Wait 5 seconds for BLE to cleanup
        vTaskDelay(5000 / portTICK_PERIOD_MS);

        // Reboot after setup
        esp_restart();
    };

    // If still setting up, wait for setup to complete
    if (!is_setup) {
        ESP_LOGI("BLE", "Waiting for setup to complete");
        for (;;) {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }

    /*  Initialize Wifi */
    std::string ssid = nvs.get("ssid");
    std::string password = nvs.get("password");

    WifiWrap wifi_wrapper;

    ESP_LOGI("SmartKnock", "Connecting to WiFi: %s with password %s", ssid.c_str(), password.c_str());

    WifiPassHeader header{ssid.c_str(), ssid.length(), password.c_str(),
                          password.length()};

    // These below lines will advertise the WiFi credentials publicly so maybe it's not a
    // great idea...
    // ble.setWifiSSID(network_name); ble.setWifiPassword(password);
    ble.onWifiCredentialsUpdated = [&](const std::string& ssid,
                                       const std::string& password) {
        // TODO: save credentials in NVS so that they're used next wakeup
        nvs.set("ssid", ssid);
        nvs.set("password", password);
        nvs.commit();
        // For now, just log it
        ESP_LOGI("SmartKnock", "Wifi credentials updated: %s, %s", ssid.c_str(),
                 password.c_str());

        // Reboot to apply new WiFi credentials
        esp_restart();
    };

    ble.onResetRequested = [&]() {
        nvs.eraseAll();
        nvs.commit();
        ESP_LOGI("SmartKnock", "Reset requested");
        esp_restart();
    };

    if (sleep_wrapper.is_wakeup_by_reset()) {
        // Reset NVS if it was a wakeup by reset (not from deep sleep)
        ESP_LOGI("SmartKnock", "Wakeup by reset");
        // nvs.eraseAll();
        // nvs.commit();
    }

    wifi_wrapper.connect(header);

    /*  Initialize Deep Sleep Handlers  */
    SleepConfig sleep_config = {
        &wifi_wrapper, GPIO_NUM_2, 1,
        10000000  // in us,
    };
    sleep_wrapper.config(sleep_config);
    sleep_wrapper.print_wakeup_reason();

    ULPConfig ulp_config = {GPIO_NUM_27, GPIO_NUM_13};
    ulp.enable_ulp_monitoring(ulp_config);

    /*  Start Tasks */
    BaseType_t xTaskAPIReturned;
    BaseType_t xTaskSleep;
    TaskHandle_t xAPIHandle = NULL;
    TaskHandle_t xSleepHandle = NULL;

    task_wifi_complete = xSemaphoreCreateBinary();

    xTaskAPIReturned =
        xTaskCreatePinnedToCore(task_scan_handler, "APIScan", GLOBAL_STACK_SIZE, &api, 1,
                                &xAPIHandle, GLOBAL_CPU_CORE_0);
    xTaskSleep =
        xTaskCreatePinnedToCore(task_sleep_handler, "SleepHandler", GLOBAL_STACK_SIZE,
                                &sleep_wrapper, 0, &xSleepHandle, GLOBAL_CPU_CORE_0);
}

void task_sleep_handler(void* pvParameters) {
    configASSERT(static_cast<DeepSleep*>(pvParameters) != nullptr);

    DeepSleep* sleep_handler = static_cast<DeepSleep*>(pvParameters);

    while (xSemaphoreTake(task_wifi_complete, (TickType_t)0) != pdTRUE) {
    }
    // Begins sleep mode after all processes are finished
    ESP_LOGI("SmartKnock", " Beginning sleep mode\n");
    // ulp.start_ulp_monitoring();
    // sleep_handler->sleep();
    for (;;) {
    }
}
// TODO:
/*
    timer wakeup periodically for sending stats
    send stats right before sleeping
*/

void task_scan_handler(void* pvParameters) {
    configASSERT(static_cast<SmartKnockAPI*>(pvParameters) != nullptr);

    TickType_t xLastWakeTime;
    SmartKnockAPI* api_handle = static_cast<SmartKnockAPI*>(pvParameters);
    xSemaphoreTake(task_wifi_complete, (TickType_t)0);
    static int num_knocks_placeholder = 0;
    for (;;) {
        xLastWakeTime = xTaskGetTickCount();
        std::string passphrase = nvs.get("passphrase");

        api_handle->send_message(passphrase, {0.1234, num_knocks_placeholder++});

        // Go through all incoming messages, then sleep when there are no more to process
        MessageType m;
        do {
            m = api_handle->get_incoming_message(passphrase);
            if (m == MessageType::LOCK) {
                ESP_LOGI("SmartKnock", "LOCK message received\n");
            }
            if (m == MessageType::UNLOCK) {
                ESP_LOGI("SmartKnock", "UNLOCK message received\n");
            }
        } while (m != MessageType::NONE);
        xSemaphoreGive(task_wifi_complete);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(15000));  // Scan every 15 s
    }
}