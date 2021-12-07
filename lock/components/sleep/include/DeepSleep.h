#pragma once

#include <stdlib.h>
#include <string>
#include <sstream>
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_sleep.h"

// Custom Components
#include "WifiWrap.h"
#include "BLE.h"
#include "esp_adc.h"
#include "ulp_adc.h"

#ifdef __cplusplus
extern "C" {
#endif

struct SleepConfig {
    WifiWrap* wifi_wrapper;
    BLE* ble_wrapper;
    gpio_num_t wakeup_pin;
    uint8_t wakeup_pin_active_level;
    uint64_t wakeup_period_us;
};

class DeepSleep {
    public:
        DeepSleep();

        void config(SleepConfig config);

        void sleep();

        void wakeup();

        esp_sleep_wakeup_cause_t get_wakeup_reason();

        void print_wakeup_reason();
        bool is_wakeup_by_reset();
    private:
        SleepConfig config_;
};

#ifdef __cplusplus
}
#endif