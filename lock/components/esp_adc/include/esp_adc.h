#pragma once

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "driver/adc.h"
#include "driver/rtc_io.h"
#include "soc/rtc.h"

#ifdef __cplusplus
extern "C" {
#endif
#define ADC_NUMBER_OF_SAMPLES   16
#define MAX_NUMBER_CHANNELS     5

class EspAdc {
    public:
        void config(adc1_channel_t ch);

        void disconnect();

        float read_channels(adc1_channel_t ch);
};

#ifdef __cplusplus
}
#endif