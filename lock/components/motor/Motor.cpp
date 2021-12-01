#include "Motor.h"

/**
 * @brief Sets up config info for motor
 * 
 * @param config 
 */
void Motor::config(const MotorConfig &config) {
    // Configure GPIO pins as outputs
    config_ = config;
    uint16_t pin_mask = 0;

    pin_mask |= (1 << config_.gpio_sleep_pin);
    pin_mask |= (1 << config_.gpio_step_pin);
    pin_mask |= (1 << config_.gpio_ms_pins[0]);
    pin_mask |= (1 << config_.gpio_ms_pins[1]);
    pin_mask |= (1 << config_.gpio_ms_pins[2]);
    pin_mask |= (1 << config_.gpio_dir_pin);
    pin_mask |= (1 << config_.gpio_enable_pin);
    pin_mask |= (1 << config_.gpio_reset_pin);

    step_pin = (gpio_num_t)config_.gpio_step_pin; 
    dir_pin = (gpio_num_t)config_.gpio_dir_pin; 
    enable_pin = (gpio_num_t)config_.gpio_enable_pin;

    gpio_config_t gpio_config_settings = {
        .pin_bit_mask = pin_mask,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&gpio_config_settings);

    // This is usually active low, may change
    gpio_set_level(enable_pin, 1); 
}

/**
 * @brief Moves motor to absolutely position from 0 to 360 degrees along the shortest path
 * 
 * @param abs_degrees 
 */
void Motor::move_motor(int abs_degrees) {

    // Set direction pin based off of degrees (+/-)
    gpio_set_level(dir_pin, (abs_degrees > 0));  

    int steps_per_revolution = 200; 
    float degrees_per_step = 360.0/abs(steps_per_revolution);
    int steps_to_complete = abs_degrees/degrees_per_step;
    
    // need steps per revolution
    // write step pin high
    // delay 
    // write step pin low
    // delay 

    // Enable (usually active low)
    gpio_set_level(enable_pin, 0);

    for (int i = 0; i < steps_to_complete; i++) {
        gpio_set_level(step_pin, 1); 
        vTaskDelay(pdMS_TO_TICKS(2));
        gpio_set_level(step_pin, 0);
        vTaskDelay(pdMS_TO_TICKS(2));
    }

    gpio_set_level(enable_pin, 1);

}

