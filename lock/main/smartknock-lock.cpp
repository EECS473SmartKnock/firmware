#include <stdio.h>

// Internal Libraries
#include "BLE.h"
#include "DeepSleep.h"
#include "NVSWrapper.h"
#include "NimBLEDevice.h"
#include "SmartKnockAPI.h"
#include "WifiWrap.h"
#include "ulp_adc.h"
#include "Motor.h"
#include "FobAuth.h"

// External Libraries
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "nvs_flash.h"

// STL libraries
#include <string>

extern "C" {
void app_main();
}

// Global Defines Variables
#define GLOBAL_STACK_SIZE 8000
#define GLOBAL_CPU_CORE_0 0
#define GLOBAL_CPU_CORE_1 1

#define GLOBAL_MAX_NUMBER_TASKS 2

// Master Task State Machine
enum Master_State {
    Initial,
    BleAuth,
    TaskScanning,
    StatPublishing,
    Sleeping
};
static const char * State_Map[] = { "Initial", "BleAuth", "TaskScanning", "StatPublishing", "Sleeping" };

SemaphoreHandle_t task_queue_sem;
SemaphoreHandle_t task_sleep_sem;

ULP ulp;
NVSWrapper nvs;
BLE ble;
Motor stepper;
/*  Initialize API Libraries    */
static DeepSleep sleep_wrapper;
static SmartKnockAPI api;

// std::string passphrase = "";  // TODO: this should be set by the setup process and
                              // saved in non-volatile storage
// Global Defines Tasks

void task_scan_handler(void* pvParameters);

void task_sleep_handler(void* pvParameters);

void task_master_handler(void* pvParameters);

void task_publish_handler(void* pvParameters);

void app_main() {
    /* ------ BLE Fob test ---------- */
    // Comment below out if you dont have fob
    ble.init();

    /*while (!ble.connectToFob());
    ble.fobWrite((const uint8_t*)"Hello world! This needs to be at least 64 bytes !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    uint8_t buffer[64];
    ble.fobRecv(buffer);
    ESP_LOGI("BLE", "Received: %s", buffer);*/

    FobAuth fa("c38dec5ff37fbee973cc03e73d13757c537ad43a3ea41ced20193df66c337a4e3926d6d6e1af6e0738c2008fcde0a71ad9d22d394f8184c11116f68e719d96bd", "10001");
    if(fa.doAuth(ble)) {
        ESP_LOGI("BLE", "Auth successful!!");
    } else {
        ESP_LOGI("BLE", "Auth failed");
    }
    // return;
    /* ------------------------------ */

    nvs.init();

    // Uncomment below if you want to test wifi manually
    
    // std::string test_ssid = "ArthurZhang";
    // std::string test_password = "arthurthesoccerball";
    // std::string test_passphrase = "rand-pass-1234";
    // nvs.set("ssid", test_ssid);
    // nvs.set("password", test_password);
    // nvs.set("passphrase", test_passphrase);
    // nvs.commit();
    // std::string temp_pass = nvs.get("passphrase");

    // ESP_LOGI("SmartKnock", "passphrase %s ", temp_pass.c_str());
    

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
        ble.startServer();
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
    /*
    MotorConfig stepper_pins = {
        .gpio_sleep_pin = GPIO_NUM_17; 
        .gpio_step_pin = GPIO_NUM_18; 
        .gpio_ms_pins = {}; 
        .gpio_dir_pin = GPIO_NUM_21; 
        .gpio_enable_pin = GPIO_NUM_22; 
        .gpio_reset_pin = GPIO_NUM_19;
    }

    stepper.config(stepper_pins);

    */
    /*  Setup Task Queue */
    // Prevent uC from sleeping
    task_sleep_sem = xSemaphoreCreateBinary();
    xSemaphoreTake(task_sleep_sem, ( TickType_t ) 0);
    // Confirm minimum number of scans are completed
    task_queue_sem = xSemaphoreCreateCounting(GLOBAL_MAX_NUMBER_TASKS, GLOBAL_MAX_NUMBER_TASKS);
    for (int i = 0; i < GLOBAL_MAX_NUMBER_TASKS; ++i) { xSemaphoreGive(task_queue_sem); }

    // TODO: start master task
    BaseType_t xTaskMaster;
    TaskHandle_t xMasterHandle = NULL;
    xTaskMaster = xTaskCreatePinnedToCore(
        task_master_handler, "MasterHandler", GLOBAL_STACK_SIZE, 
        nullptr, 0, &xMasterHandle, GLOBAL_CPU_CORE_0
    );
}

void task_master_handler(void* pvParameters) {
    configASSERT(pvParameters == nullptr);

    BaseType_t xTaskScanReturned, xTaskSleep;
    BaseType_t xTaskPublishStats;
    TaskHandle_t xAPIHandle = NULL, xSleepHandle = NULL, xPublishHandle = NULL;
    esp_sleep_wakeup_cause_t sleep_wakeup_reason = sleep_wrapper.get_wakeup_reason();
    Master_State state = Initial;
    TickType_t xLastWakeTime;

    int num_knocks = 0;
    if (!nvs.existsStr("num_knocks")) {
        nvs.set("num_knocks", std::to_string(num_knocks));
        nvs.commit();
    }

    xTaskScanReturned = xTaskCreatePinnedToCore(
        task_scan_handler, "APIScan", GLOBAL_STACK_SIZE, &api, 1, &xAPIHandle, GLOBAL_CPU_CORE_0
    );

    xTaskSleep = xTaskCreatePinnedToCore(
        task_sleep_handler, "SleepHandler", GLOBAL_STACK_SIZE, 
        &sleep_wrapper, 0, &xSleepHandle, GLOBAL_CPU_CORE_0
    );

    xTaskPublishStats = xTaskCreatePinnedToCore(
        task_publish_handler, "PublishScan", GLOBAL_STACK_SIZE, 
        NULL, 1, &xPublishHandle, GLOBAL_CPU_CORE_0
    );

    for (;;) {
        xLastWakeTime = xTaskGetTickCount();
        ESP_LOGI("SmartKnock", " current state %s\n", State_Map[int(state)]);
        switch (state) {
            case Initial: {
                if (sleep_wakeup_reason==ESP_SLEEP_WAKEUP_ULP) {
                    state = BleAuth;
                } else {
                    state = TaskScanning;
                }
                break;
            }
            case BleAuth: {
                // TODO: Add ble task code here
                num_knocks = stoi(nvs.get("num_knocks"));
                ++num_knocks;
                nvs.set("num_knocks", std::to_string(num_knocks));
                nvs.commit();

                state = TaskScanning;
                break;
            }
            case TaskScanning: {
                if (uxSemaphoreGetCount(task_queue_sem) < 2) {
                    state = StatPublishing;
                }
                break;
            }
            case StatPublishing: {
                // Replace with this function and battery voltage for 0.1234 and num_knocks_placeholder
                if (uxSemaphoreGetCount(task_queue_sem) < 1) {
                    state = Sleeping;
                }
                
                break;
            }
            case Sleeping: {
                xSemaphoreGive(task_sleep_sem);
                break;
            }
            default: {
                ESP_LOGI("SmartKnock", " master task should not be here\n");
                break;
            }
        }
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(100));  // Scan every 0.1 s
    }
    
}

void task_publish_handler(void* pvParameters) {
    configASSERT(pvParameters == nullptr);

    while (uxSemaphoreGetCount(task_queue_sem) > 1) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    int num_knocks = stoi(nvs.get("num_knocks"));
    ESP_LOGI("SmartKnock", " number knocks %i \n", num_knocks);
    
    std::string passphrase = nvs.get("passphrase");
    api.send_message(passphrase, {0.1234, num_knocks});
    xSemaphoreTake(task_queue_sem, (TickType_t) 0);

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(15000));
    }
}

void task_sleep_handler(void* pvParameters) {
    configASSERT(static_cast<DeepSleep*>(pvParameters) != nullptr);

    DeepSleep* sleep_handler = static_cast<DeepSleep*>(pvParameters);

    while (xSemaphoreTake(task_sleep_sem, ( TickType_t ) 0) != pdTRUE) {
    }
    // Begins sleep mode after all processes are finished
    ESP_LOGI("SmartKnock", " Beginning sleep mode\n");
    ulp.start_ulp_monitoring();
    sleep_handler->sleep();
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
    for (;;) {
        // Block task if only one semaphore is left
        while (uxSemaphoreGetCount(task_queue_sem) <= 1) {
            vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(15000));
        }
        xLastWakeTime = xTaskGetTickCount();

        std::string passphrase = nvs.get("passphrase");

        // Go through all incoming messages, then sleep when there are no more to process
        MessageType m;
        do {
            m = api_handle->get_incoming_message(passphrase);
            if (m == MessageType::LOCK) {
                ESP_LOGI("SmartKnock", "LOCK message received\n");
                // stepper.move_motor(90);
            } else if (m == MessageType::UNLOCK) {
                ESP_LOGI("SmartKnock", "UNLOCK message received\n");
                // stepper.move_motor(-90);
            }
        } while (m != MessageType::NONE);

        xSemaphoreTake(task_queue_sem, (TickType_t)0);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(15000));  // Scan every 15 s
    }
}