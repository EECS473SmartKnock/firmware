#pragma once

#include <stdlib.h>
#include <string>
#include <sstream>
#include "driver/gpio.h"

// Custom Components
#include "WifiWrap.h"

#ifdef __cplusplus
extern "C" {
#endif

struct SleepConfig {
    WifiWrap* wifi_wrapper;
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

        void print_wakeup_reason();
    private:
        WifiWrap* wifi_wrapper_;
};

#ifdef __cplusplus
}
#endif