#include "ulp_adc.h"

extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[]   asm("_binary_ulp_main_bin_end");

/*
 * Offset (in 32-bit words) in RTC Slow memory where the data is placed
 * by the ULP coprocessor. It can be chosen to be any value greater or equal
 * to ULP program size, and less than the CONFIG_ESP32_ULP_COPROC_RESERVE_MEM/4 - 6,
 * where 6 is the number of words used by the ULP coprocessor.
 */
// #define ULP_DATA_OFFSET     36

// _Static_assert(ULP_DATA_OFFSET < CONFIG_ESP32_ULP_COPROC_RESERVE_MEM/4 - 6,
//         "ULP_DATA_OFFSET is set too high, or CONFIG_ESP32_ULP_COPROC_RESERVE_MEM is not sufficient");

void ULP::enable_ulp_monitoring(ULPConfig config) {
    config_ = config;
    // Allows ulp to wakeup main processor
    esp_sleep_enable_ulp_wakeup();
    
     ESP_ERROR_CHECK(
         ulp_load_binary(0, ulp_main_bin_start, (ulp_main_bin_end-ulp_main_bin_start) / sizeof(uint32_t))
     );

    rtc_gpio_init(config.ulp_pin1);
    rtc_gpio_set_direction(config.ulp_pin1, RTC_GPIO_MODE_OUTPUT_ONLY);

    rtc_gpio_init(config.ulp_pin2);
    rtc_gpio_set_direction(config.ulp_pin2, RTC_GPIO_MODE_OUTPUT_ONLY);

    /* Configure ADC channel */
    /* Note: when changing channel here, also change 'adc_channel' constant
        in adc.S */
    adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11);
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_ulp_enable();
    ulp_low_threshold = 1;//0.001 * (4095 / 3.3);  // 2.5 volt lower bound
    ulp_high_threshold = 1;//0.001 * (4095 / 3.3);   // 2.5 volts upper bound

    /* Set ULP wake up period to 100ms */
    ulp_set_wakeup_period(0, 1 * 1000);
}

void ULP::start_ulp_monitoring() {
    ESP_ERROR_CHECK( ulp_run((&ulp_entry - RTC_SLOW_MEM) / sizeof(uint32_t)) );
}