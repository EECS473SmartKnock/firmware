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
#include "driver/gpio.h"
#include "driver/mcpwm.h"

struct MotorConfig {
    uint64_t gpio_sleep_pin;
    uint64_t gpio_step_pin;
    uint64_t gpio_dir_pin;
    uint64_t gpio_enable_pin;
    uint64_t gpio_reset_pin;
};

class Motor {
    public:
        void config(const MotorConfig &config);

        void set_next_degrees(int abs_degrees) {
            next_abs_degree_ = abs_degrees;
        }

        int get_next_degrees() {
            return next_abs_degree_;
        }
        // Starts freertos task to step motor until completion
        void move_motor(int abs_degrees);
    private:
        MotorConfig config_;
        int next_abs_degree_;
};