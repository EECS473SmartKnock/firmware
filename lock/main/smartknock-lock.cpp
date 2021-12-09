#include <stdio.h>

// Internal Libraries
#include "BLE.h"
#include "DeepSleep.h"
#include "FobAuth.h"
#include "Motor.h"
#include "NVSWrapper.h"
#include "NimBLEDevice.h"
#include "SmartKnockAPI.h"
#include "WifiWrap.h"
#include "esp_adc.h"
#include "ulp_adc.h"

// External Libraries
#include "driver/gpio.h"
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
#define GLOBAL_STACK_SIZE 32000
#define GLOBAL_CPU_CORE_0 0
#define GLOBAL_CPU_CORE_1 1

static constexpr const char* SMARTKNOCK_STATE_KEY = "lock_state";
static constexpr const int SMARTKNOCK_LOCK_STATE = 0;
static constexpr const int SMARTKNOCK_UNLOCK_STATE = 1;

static constexpr const int LOCK_TO_UNLOCK_ANGLE = -200;
static constexpr const int UNLOCK_TO_LOCK_ANGLE = 300;

void stepper_lock_helper(MessageType m = MessageType::NONE);

#define GLOBAL_MAX_NUMBER_TASKS 3

// Master Task State Machine
enum Master_State { Initial, HandleRequests, StatPublishing, Waiting, Sleeping };
static const char* State_Map[] = {"Initial", "HandleRequests", "StatPublishing",
                                  "Waiting", "Sleeping"};

SemaphoreHandle_t task_queue_sem;
SemaphoreHandle_t task_sleep_sem;
SemaphoreHandle_t task_move_motor_sem;

static ULP ulp;
NVSWrapper nvs;
BLE ble;
Motor stepper;
EspAdc adc;
FobAuth fa(
    "c38dec5ff37fbee973cc03e73d13757c537ad43a3ea41ced20193df66c337a4e3926d6d6e1af6e0738c2"
    "008fcde0a71ad9d22d394f8184c11116f68e719d96bd",
    "10001");
// FobAuth fa("c38dec5ff37fbee973cc03e73d13757c537ad43a3ea41ced20193df66c337a4e",
// "10001");
// FobAuth
// fa("00bb41cb594985b095f2506037019d1dc97c8033b8fb8f9b1629a5adfbeecbb0f406cf5cf140ec3ed53f81c15df584ad22a22196131e0c7f6c6766dd804e9b2681",
// "10001");

/*  Initialize API Libraries    */
static DeepSleep sleep_wrapper;
SmartKnockAPI api;

// std::string passphrase = "";  // TODO: this should be set by the setup process and
// saved in non-volatile storage
// Global Defines Tasks

void task_fob_handler(void* pvParameters);

void task_scan_handler(void* pvParameters);

void task_sleep_handler(void* pvParameters);

void task_master_handler(void* pvParameters);

void task_publish_handler(void* pvParameters);

void task_motor_handler(void* pvParameters);

void app_main() {
    /* ------ BLE Fob test ---------- */
    // Comment below out if you dont have fob

    // ble.init();

    /*while (!ble.connectToFob());
    ble.fobWrite((const uint8_t*)"Hello world! This needs to be at least 64 bytes
    !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"); uint8_t buffer[64];
    ble.fobRecv(buffer);
    ESP_LOGI("BLE", "Received: %s", buffer);*/

    // MotorConfig stepper_pins = {
    //     GPIO_NUM_17,
    //     GPIO_NUM_18,
    //     GPIO_NUM_21,
    //     GPIO_NUM_22,
    //     GPIO_NUM_19
    // };
    // stepper.config(stepper_pins);
    // if(fa.doAuth(ble)) {
    //     ESP_LOGE("BLE", "Auth successful!!");
    //     stepper.set_next_degrees(90);
    //     task_motor_handler(nullptr);
    // } else {
    //     ESP_LOGE("BLE", "Auth failed");
    // }

    // return;
    /* ------------------------------ */

    nvs.init();
    // nvs.eraseAll();
    // nvs.commit();

    // Uncomment below if you want to test wifi manually

    // std::string test_ssid = "ArthurZhang";
    // std::string test_password = "arthurthes";
    // std::string test_passphrase = "CatH0use";
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

    /* Check if lock state exists, if not default to locked */
    if (!nvs.exists(SMARTKNOCK_STATE_KEY)) {
        nvs.set(SMARTKNOCK_STATE_KEY, SMARTKNOCK_LOCK_STATE);
        nvs.commit();
    }

    bool is_setup = nvs.existsStr("passphrase");
    if (is_setup) {
        std::string passphrase = nvs.get("passphrase");
        ESP_LOGI("SmartKnock", "Lock is set up and ready to go! Passphrase: %s",
                 passphrase.c_str());
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

    ESP_LOGI("SmartKnock", "Connecting to WiFi: %s with password %s", ssid.c_str(),
             password.c_str());

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
        &wifi_wrapper, &ble, GPIO_NUM_2, 1,
        10000000  // in us,
    };
    sleep_wrapper.config(sleep_config);
    sleep_wrapper.print_wakeup_reason();

    MotorConfig stepper_pins = {GPIO_NUM_17, GPIO_NUM_18, GPIO_NUM_21, GPIO_NUM_22,
                                GPIO_NUM_19};

    stepper.config(stepper_pins);

    /* Initialize ADC channels */
    adc.config(ADC1_CHANNEL_5);

    /*  Setup Task Queue */
    // Prevent motor task from running
    task_move_motor_sem = xSemaphoreCreateBinary();
    xSemaphoreTake(task_move_motor_sem, (TickType_t)0);

    // Prevent uC from sleeping
    task_sleep_sem = xSemaphoreCreateBinary();
    xSemaphoreTake(task_sleep_sem, (TickType_t)0);
    // Confirm minimum number of scans are completed
    task_queue_sem =
        xSemaphoreCreateCounting(GLOBAL_MAX_NUMBER_TASKS, GLOBAL_MAX_NUMBER_TASKS);
    for (int i = 0; i < GLOBAL_MAX_NUMBER_TASKS; ++i) {
        xSemaphoreGive(task_queue_sem);
    }

    // TODO: start master task
    BaseType_t xTaskMaster;
    TaskHandle_t xMasterHandle = NULL;
    xTaskMaster =
        xTaskCreatePinnedToCore(task_master_handler, "MasterHandler", GLOBAL_STACK_SIZE,
                                nullptr, 0, &xMasterHandle, GLOBAL_CPU_CORE_0);
}

void task_master_handler(void* pvParameters) {
    configASSERT(pvParameters == nullptr);

    BaseType_t xTaskAPIReturned, xTaskSleep, xTaskPublishStats, xTaskFobHandle;
    TaskHandle_t xAPIHandle = NULL, xSleepHandle = NULL, xPublishHandle = NULL,
                 xFobHandle = NULL;
    esp_sleep_wakeup_cause_t sleep_wakeup_reason = sleep_wrapper.get_wakeup_reason();
    Master_State state = Initial;
    TickType_t xLastWakeTime, xFunctionStartTime, xWaitStartTime;

    int num_knocks = 0;
    if (!nvs.existsStr("num_knocks")) {
        nvs.set("num_knocks", std::to_string(num_knocks));
        nvs.commit();
    }

    // xTaskSleep = xTaskCreatePinnedToCore(
    //     task_sleep_handler, "SleepHandler", GLOBAL_STACK_SIZE,
    //     &sleep_wrapper, 1, &xSleepHandle, GLOBAL_CPU_CORE_0
    // );

    // xTaskPublishStats = xTaskCreatePinnedToCore(
    //     task_publish_handler, "PublishScan", GLOBAL_STACK_SIZE,
    //     NULL, 2, &xPublishHandle, GLOBAL_CPU_CORE_0
    // );

    xFunctionStartTime = xTaskGetTickCount();
    for (;;) {
        xLastWakeTime = xTaskGetTickCount();
        // ESP_LOGI("Master", " current state %s\n", State_Map[int(state)]);
        switch (state) {
            case Initial: {
                if (sleep_wakeup_reason == ESP_SLEEP_WAKEUP_ULP) {
                    num_knocks = stoi(nvs.get("num_knocks"));
                    ++num_knocks;
                    nvs.set("num_knocks", std::to_string(num_knocks));
                    nvs.commit();
                    // Create tasks that are blocked but higher importance than master
                    // loop
                    xTaskFobHandle = xTaskCreatePinnedToCore(
                        task_fob_handler, "FobHandle", GLOBAL_STACK_SIZE, nullptr, 1,
                        &xFobHandle, GLOBAL_CPU_CORE_0);
                } else {
                    xSemaphoreTake(task_queue_sem, (TickType_t)0);
                }

                xTaskAPIReturned = xTaskCreatePinnedToCore(
                    task_scan_handler, "APIScan", GLOBAL_STACK_SIZE, &api, 1, &xAPIHandle,
                    GLOBAL_CPU_CORE_0);

                // Release Tasks
                xSemaphoreTake(task_queue_sem, (TickType_t)0);

                state = HandleRequests;
                break;
            }
            case HandleRequests: {
                // ESP_LOGI("Master", " number tasks remaining %i\n",
                // uxSemaphoreGetCount(task_queue_sem));
                if (uxSemaphoreGetCount(task_queue_sem) < 1) {
                    if (xTaskFobHandle == pdTRUE) {
                        ESP_LOGI("Master", " deleting fob task\n");
                        vTaskDelete(xFobHandle);
                    }
                    ESP_LOGI("Master", " deleting api task\n");
                    vTaskDelete(xAPIHandle);
                    state = StatPublishing;
                }

                break;
            }
            case StatPublishing: {
                task_publish_handler(nullptr);
                xWaitStartTime = xTaskGetTickCount();
                state = Waiting;
                ble.startServer();
                break;
            }
            case Waiting: {  // make this wait after 15 seconds from when waiting began
                if (xTaskGetTickCount() - xWaitStartTime > pdMS_TO_TICKS(15000)) {
                    state = Sleeping;
                }
                break;
            }
            case Sleeping: {
                stepper_lock_helper(MessageType::LOCK);
                vTaskDelay(pdMS_TO_TICKS(5000)); // wait 5 seconds before sleeping so that we don't wake ourselves up from doing the final lock
                task_sleep_handler(&sleep_wrapper);
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

void task_fob_handler(void* pvParameters) {
    configASSERT(pvParameters == nullptr);

    while (uxSemaphoreGetCount(task_queue_sem) > 1) {
        vTaskDelay(200);  // Block until released
    }

    if (fa.doAuth(ble)) {
        ESP_LOGI("BLE", "Auth successful!!");
        stepper_lock_helper(MessageType::NONE);
    } else {
        ESP_LOGI("BLE", "Auth failed");
    }

    ESP_LOGI("BLE", "Fob task completed\n");
    xSemaphoreTake(task_queue_sem, (TickType_t)0);  // Task completed
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(30000));
    }  // Dont do anything for 15 seconds until task destruction
}

void task_motor_handler(void* pvParameters) {
    configASSERT(pvParameters == nullptr);

    // for (;;) {
    //     while(xSemaphoreTake(task_move_motor_sem, ( TickType_t ) 0) != pdTRUE) {
    //         vTaskDelay(pdMS_TO_TICKS(10));
    //     }
    ESP_LOGI("SmartKnock", " Moving motor %i degrees\n", stepper.get_next_degrees());
    stepper.move_motor(stepper.get_next_degrees());
    // }
}

void task_publish_handler(void* pvParameters) {
    configASSERT(pvParameters == nullptr);

    // while (uxSemaphoreGetCount(task_queue_sem) > 1) {
    //     vTaskDelay(pdMS_TO_TICKS(10));
    // }
    int num_knocks = stoi(nvs.get("num_knocks"));
    float battery_voltage = adc.read_channels(ADC1_CHANNEL_5);
    ESP_LOGI("SmartKnock", " number knocks %i, battery voltage %0.5f\n", num_knocks,
             battery_voltage);

    std::string passphrase = nvs.get("passphrase");
    api.send_message(passphrase, {battery_voltage, num_knocks});
    // xSemaphoreTake(task_queue_sem, (TickType_t) 0);

    // for (;;) {
    //     vTaskDelay(pdMS_TO_TICKS(15000));
    // }
}

void task_sleep_handler(void* pvParameters) {
    configASSERT(static_cast<DeepSleep*>(pvParameters) != nullptr);

    DeepSleep* sleep_handler = static_cast<DeepSleep*>(pvParameters);

    // while (xSemaphoreTake(task_sleep_sem, ( TickType_t ) 0) != pdTRUE) {
    //     vTaskDelay(pdMS_TO_TICKS(10));
    // }
    // Begins sleep mode after all processes are finished
    ESP_LOGI("SmartKnock", " Beginning sleep mode\n");
    adc.disconnect();
    /* Initialize ULP monitoring config pins */
    ULPConfig ulp_config = {GPIO_NUM_27, GPIO_NUM_13};
    ulp.enable_ulp_monitoring(ulp_config);
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

    while (uxSemaphoreGetCount(task_queue_sem) == GLOBAL_MAX_NUMBER_TASKS) {
        vTaskDelay(200);  // Block until released
    }
    xLastWakeTime = xTaskGetTickCount();

    std::string passphrase = nvs.get("passphrase");

    // Go through all incoming messages, then sleep when there are no more to process
    MessageType m = MessageType::LOCK;
    do {
        m = api_handle->get_incoming_message(passphrase);
        if (m == MessageType::LOCK) {
            ESP_LOGI("SmartKnock", "LOCK message received\n");
            // xSemaphoreGive(task_move_motor_sem); // releases motor handler task
            stepper_lock_helper(m);
            xSemaphoreTake(task_queue_sem, (TickType_t)0);
        } else if (m == MessageType::UNLOCK) {
            ESP_LOGI("SmartKnock", "UNLOCK message received\n");
            // xSemaphoreGive(task_move_motor_sem); // releases motor handler task
            stepper_lock_helper(m);
            xSemaphoreTake(task_queue_sem, (TickType_t)0);
        }
    } while (m != MessageType::NONE);

    xSemaphoreTake(task_queue_sem, (TickType_t)0);
    ESP_LOGI("SmartKnock", "API task completed\n");
    for (;;) {
        vTaskDelayUntil(
            &xLastWakeTime,
            pdMS_TO_TICKS(30000));  // Wait for 30 seconds until task is deleted
    }
}

void stepper_lock_helper(MessageType m) {
    // If MessageType is NONE, toggle the state of the lock
    // If it's LOCK, lock the door if it's currently unlocked and update the state
    // If it's UNLOCK, unlock the door if it's currently locked and update the state
    int lock_state = SMARTKNOCK_LOCK_STATE;
    if (nvs.exists(SMARTKNOCK_STATE_KEY)) {
        lock_state = nvs.get<int>(SMARTKNOCK_STATE_KEY);
    }

    int new_lock_state = lock_state;
    
    if (m == MessageType::NONE) {
        // Toggle lock position if activated from fob
        if (lock_state == SMARTKNOCK_LOCK_STATE) {
            stepper.set_next_degrees(LOCK_TO_UNLOCK_ANGLE);
            new_lock_state = SMARTKNOCK_UNLOCK_STATE;
        } else {
            stepper.set_next_degrees(UNLOCK_TO_LOCK_ANGLE);
            new_lock_state = SMARTKNOCK_LOCK_STATE;
        }

    } else {
        // From api
        if (lock_state == SMARTKNOCK_UNLOCK_STATE) {
            // Lock
            if (m == MessageType::LOCK) {
                ESP_LOGI("SmartKnock", "Lock was unlocked, now locking it\n");
                stepper.set_next_degrees(UNLOCK_TO_LOCK_ANGLE);
                new_lock_state = SMARTKNOCK_LOCK_STATE;
            } else if (m == MessageType::UNLOCK) {
                ESP_LOGI("SmartKnock", "Lock was unlocked, not doing anything\n");
                stepper.set_next_degrees(0);
            }
        }
        else if (lock_state == SMARTKNOCK_LOCK_STATE) {
            // Unlock
            if (m == MessageType::UNLOCK) {
                ESP_LOGI("SmartKnock", "Lock was locked, now unlocking it\n");
                stepper.set_next_degrees(LOCK_TO_UNLOCK_ANGLE);
                new_lock_state = SMARTKNOCK_UNLOCK_STATE;
            } else if (m == MessageType::LOCK) {
                ESP_LOGI("SmartKnock", "Lock was locked, not doing anything\n");
                stepper.set_next_degrees(0);
            }
        }
    }
    // Store new_lock_state in NVS
    nvs.set(SMARTKNOCK_STATE_KEY, new_lock_state);
    task_motor_handler(nullptr);
    vTaskDelay(pdMS_TO_TICKS(1000));
}