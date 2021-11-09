#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "sdkconfig.h"
#include "soc/soc_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "driver/adc.h"
#include "driver/rtc_io.h"
#include "soc/rtc.h"

#if CONFIG_IDF_TARGET_ESP32
#include "esp32/ulp.h"
#endif

struct ULPConfig {
    gpio_num_t ulp_pin1;
    gpio_num_t ulp_pin2;
};

extern uint32_t ulp_entry;
extern uint32_t ulp_sample_counter;
extern uint32_t ulp_low_threshold;
extern uint32_t ulp_high_threshold;
extern uint32_t ulp_ADC_reading;

class ULP {
    public:
        ULP();

        void enable_ulp_monitoring(ULPConfig config);

        void start_ulp_monitoring();
    private:
        ULPConfig config_;
};

#ifdef __cplusplus
}
#endif