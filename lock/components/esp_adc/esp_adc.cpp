#include "esp_adc.h"

void EspAdc::config(adc1_channel_t ch) {
    // Set ADC 1 resolution to 12 bits
    ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_12));
    // Set channel atten
    ESP_ERROR_CHECK(adc1_config_channel_atten(ch, ADC_ATTEN_DB_11));

}

void EspAdc::disconnect() {
    adc1_ulp_enable();
}

// Returns number of channels read
float EspAdc::read_channels(adc1_channel_t ch) {
    uint32_t adc_reading = 0;
    for (int i = 0; i < ADC_NUMBER_OF_SAMPLES; ++i) {
        adc_reading += adc1_get_raw(ch);
    }
    adc_reading /= ADC_NUMBER_OF_SAMPLES;

    // Convert adc reading to voltage in mV
    float voltage = adc_reading * 0.00293 * 1.13852; // first number is hardware cal, second is error cal using two point method
    return voltage;
}