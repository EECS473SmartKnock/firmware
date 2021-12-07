#include "Motor.h"

static const int STEPS_PER_REV  = 3100;
PRIVILEGED_DATA static portMUX_TYPE xTaskQueueMutex = portMUX_INITIALIZER_UNLOCKED;

/**
 * @brief Sets up config info for motor
 * 
 * @param config 
 */
void Motor::config(const MotorConfig &config) {
    // Configure GPIO pins as outputs
    config_ = config;
    gpio_config_t io_conf;
    // Set sleep
    io_conf = {
        BIT(config_.gpio_sleep_pin),
        GPIO_MODE_OUTPUT,
        GPIO_PULLUP_ENABLE, 
        GPIO_PULLDOWN_DISABLE,
        GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    // Set step low default
    io_conf = {
        BIT(config_.gpio_step_pin),
        GPIO_MODE_OUTPUT,
        GPIO_PULLUP_DISABLE, 
        GPIO_PULLDOWN_DISABLE,
        GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    // Set dir pin default
    io_conf = {
        BIT(config_.gpio_dir_pin),
        GPIO_MODE_OUTPUT,
        GPIO_PULLUP_DISABLE, 
        GPIO_PULLDOWN_ENABLE,
        GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    // Set enable low deafault
    io_conf = {
        BIT(config_.gpio_enable_pin),
        GPIO_MODE_OUTPUT,
        GPIO_PULLUP_DISABLE, 
        GPIO_PULLDOWN_ENABLE,
        GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    // Set reset high (not in reset)
    io_conf = {
        BIT(config_.gpio_reset_pin),
        GPIO_MODE_OUTPUT,
        GPIO_PULLUP_ENABLE, 
        GPIO_PULLDOWN_DISABLE,
        GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    // Set reset pin high enable step inputs
    ESP_ERROR_CHECK(gpio_set_level((gpio_num_t) config_.gpio_reset_pin, 1)); 

    // Set gpio sleep high to turn on
    ESP_ERROR_CHECK(gpio_set_level((gpio_num_t) config_.gpio_sleep_pin, 1)); 

    // Set gpio step pin low to get ready for stepping
    ESP_ERROR_CHECK(gpio_set_level((gpio_num_t) config_.gpio_step_pin, 0)); 

    // Set gpio enable pin high to not enable
    ESP_ERROR_CHECK(gpio_set_level((gpio_num_t) config_.gpio_enable_pin, 1)); 
}

/**
 * @brief Moves motor to absolutely position from 0 to 360 degrees along the shortest path
 * 
 * @param abs_degrees 
 */
void Motor::move_motor(int abs_degrees) {

    // Enable (usually active low)
    gpio_set_level((gpio_num_t) config_.gpio_enable_pin, 0);
    
    // Set direction pin based off of degrees (+/-)
    gpio_set_level((gpio_num_t) config_.gpio_dir_pin, (abs_degrees > 0));  

    float degrees_per_step = 360.0/abs(STEPS_PER_REV);
    int steps_to_complete = abs_degrees/degrees_per_step;
    
    ESP_LOGI("Motor ", "moving %i steps\n", steps_to_complete);
    
    vTaskDelay(pdMS_TO_TICKS(1));
    for (int i = 0; i < steps_to_complete; i++) {
        gpio_set_level((gpio_num_t) config_.gpio_step_pin, 1); 
        for (int i = 0; i < 1000; ++i) {asm("nop");}
        // vTaskDelay(1);
        gpio_set_level((gpio_num_t) config_.gpio_step_pin, 0);
        for (int i = 0; i < 1000; ++i) {asm("nop");}
        // vTaskDelay(1);
    }

    // Disable chip
    gpio_set_level((gpio_num_t) config_.gpio_enable_pin, 1);
}

