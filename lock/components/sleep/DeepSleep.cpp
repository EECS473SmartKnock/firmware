#include <stdio.h>

// ESP libraries
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_sleep.h"
#include "driver/rtc_io.h"

#include "DeepSleep.h"

static const char *TAG = "deep sleep";

RTC_DATA_ATTR static int wake_count = 0;

DeepSleep::DeepSleep() {}

/**
 * @brief Initialize deep sleep mode for RTC gpio pin wakeup
 * 
 *  @param wakeup_pin Must be gpio pins 0,2,4,12-15,25-27,32-39
 * 
 */
void DeepSleep::config(SleepConfig config)
{
    ESP_ERROR_CHECK(rtc_gpio_pullup_en(config.wakeup_pin));
    ESP_ERROR_CHECK(rtc_gpio_isolate(config.wakeup_pin));
    // Enables wakeup from timer source
    // ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(config.wakeup_period_us));
    // Enables wakeup from RTC pin source
    // ESP_ERROR_CHECK(esp_sleep_enable_ext0_wakeup(config.wakeup_pin, config.wakeup_pin_active_level));
    wifi_wrapper_ = config.wifi_wrapper;
}

void DeepSleep::sleep()
{
    // Disable Wifi
    wifi_wrapper_->disconnect();
    // TODO: Disable BLE

    // Starts Deep Sleep
    esp_deep_sleep_start();
}

void DeepSleep::wakeup()
{
    
}

void DeepSleep::print_wakeup_reason()
{
    switch(esp_sleep_get_wakeup_cause()) {
        case ESP_SLEEP_WAKEUP_EXT0: {
            ESP_LOGI(TAG, " woken up by external gpio");
            break;
        }
        case ESP_SLEEP_WAKEUP_TIMER: {
            ESP_LOGI(TAG, " woken up by periodic timer");
            break;
        }
        case ESP_SLEEP_WAKEUP_ULP: {
            ESP_LOGI(TAG, " woken up by adc read");
            break;
        }
        default: {
            ESP_LOGI(TAG, " woken up by reset");
            break;
        }
    }
}

bool DeepSleep::is_wakeup_by_reset() {
    return esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_UNDEFINED;
}

void RTC_IRAM_ATTR esp_wake_deep_sleep(void) {
    esp_default_wake_deep_sleep();
    // Add additional functionality here
    static RTC_RODATA_ATTR const char wakeup_str[] = "deep sleep woken up, Wakeup count: %d\n";
    esp_rom_printf(wakeup_str, wake_count++);
}