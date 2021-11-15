/**
 * @brief This library is for the A4988 Stepper Motor Driver
 * 
 *        Datasheet: https://www.pololu.com/file/0J450/a4988_DMOS_microstepping_driver_with_translator.pdf
 * 
 */
#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/mcpwm.h"

struct MotorConfig {
    // mcpwm_unit_t pwm_unit_num;
    // mcpwm_io_signals_t pwm_output_channel;
    // int gpio_output_num;
    // mcpwm_timer_t pwm_timer;
    // mcpwm_config_t pwm_config;

    int gpio_sleep_pin;
    int gpio_step_pin;
    int gpio_ms_pins[3];
    int gpio_dir_pin;
    int gpio_enable_pin;
    int gpio_reset_pin;
};

class Motor {
    public:
        void config(MotorConfig config);

        // Starts freertos task to step motor until completion
        void move_motor(int abs_degrees);
    private:
        MotorConfig config_;
};