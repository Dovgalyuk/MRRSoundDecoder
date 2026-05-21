/*
CVs

CV 2    Start volt
CV 3	Acceleration Rate (full speed for 0.896*CV3 seconds)
CV 4	Deceleration Rate

*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "driver/gpio.h"

#include "cv.h"
#include "engine.h"
#include "vm.h"
#include "variables.h"
#include "pins.h"
#include "logger.h"

#define ENGINE_OUTPUT_FWD_LIGHT  PHYS_OUTPUT1
#define ENGINE_OUTPUT_BACK_LIGHT PHYS_OUTPUT2

#define MOTOR_SPEED_MODE        LEDC_LOW_SPEED_MODE
#define MOTOR_CHANNEL1          LEDC_CHANNEL_0
#define MOTOR_CHANNEL2          LEDC_CHANNEL_1
#define MOTOR_TIMER             LEDC_TIMER_0
#define MOTOR_PWM_FREQUENCY     40000
#define MOTOR_PWM_RESOLUTION    LEDC_TIMER_8_BIT
#define MOTOR_PWM_MAX           255

#define OUT_SPEED_MODE          LEDC_LOW_SPEED_MODE
#define OUT_PWM_FREQUENCY       40000
#define OUT_PWM_RESOLUTION      LEDC_TIMER_8_BIT
#define OUT_PWM_MAX             255
#define OUT_TIMER               LEDC_TIMER_1

#define OUT_PWM_PINS            2

static const uint8_t pwm_outputs[OUT_PWM_PINS] = {ENGINE_OUTPUT_FWD_LIGHT, ENGINE_OUTPUT_BACK_LIGHT};
static const uint8_t pwm_pins[OUT_PWM_PINS] = {PHYS_OUTPUT_FWD_LIGHT, PHYS_OUTPUT_BACK_LIGHT};
static const uint8_t pwm_pin_channels[OUT_PWM_PINS] = {LEDC_CHANNEL_2, LEDC_CHANNEL_3};
static bool pwm_pin_states[OUT_PWM_PINS];

static void engine_task(void *args)
{
    while (true) {
        /* Update speed */
        uint8_t speed = engine_get_speed();
        uint8_t s1 = MOTOR_PWM_MAX;
        uint8_t s2 = MOTOR_PWM_MAX;
        if (speed) {
            bool dir = engine_get_direction();
            uint16_t min = cv_read(CV_VSTART);
            if (!dir) {
                min = cv_read(CV_REVERSE_VSTART);
            }
            uint8_t max_speed = 255;
            if (vm_get_var(F_SWITCHING)) {
                max_speed = max_speed * cv_read(CV_SWITCHING_TRIM) / 128;
            }
            uint8_t range = min < max_speed ? max_speed - min : min;
            // Drive 0 output as PWM
            uint8_t pwm = MOTOR_PWM_MAX - (min + (range * speed) / MOTOR_PWM_MAX);
            // FWD 10
            // REV 01
            if (dir) {
                s2 = pwm;
            } else {
                s1 = pwm;
            }
        }
        ledc_set_duty(MOTOR_SPEED_MODE, MOTOR_CHANNEL1, s1);
        ledc_set_duty(MOTOR_SPEED_MODE, MOTOR_CHANNEL2, s2);
        ledc_update_duty(MOTOR_SPEED_MODE, MOTOR_CHANNEL1);
        ledc_update_duty(MOTOR_SPEED_MODE, MOTOR_CHANNEL2);

        /* Update smoke */
        gpio_set_level(PHYS_OUTPUT_SMOKE, 0);

        /* Update LEDs */
        for (int i = 0 ; i < OUT_PWM_PINS ; ++i) {
            bool cur = vm_get_var(pwm_outputs[i]);
            if (cur != pwm_pin_states[i]) {
                const OutputProps *p = engine_get_output_props(pwm_outputs[i]);
                uint32_t delay = cur ? p->delay_on : p->delay_off;
                delay *= 1000;
                pwm_pin_states[i] = cur;
                ledc_set_fade_with_time(OUT_SPEED_MODE, pwm_pin_channels[i],
                                        cur > 0 ? OUT_PWM_MAX : 0, delay);
                ledc_fade_start(OUT_SPEED_MODE, pwm_pin_channels[i],
                                LEDC_FADE_NO_WAIT);
            }
        }

        /* Wait */
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void engine_init(void)
{
    /* Setup PWM pins for motor driver */
    ledc_timer_config_t ledc_timer_motor = {
        .speed_mode       = MOTOR_SPEED_MODE,
        .duty_resolution  = MOTOR_PWM_RESOLUTION,
        .timer_num        = MOTOR_TIMER,
        .freq_hz          = MOTOR_PWM_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer_motor));
    ledc_timer_config_t ledc_timer_out = {
        .speed_mode       = OUT_SPEED_MODE,
        .duty_resolution  = OUT_PWM_RESOLUTION,
        .timer_num        = OUT_TIMER,
        .freq_hz          = OUT_PWM_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer_out));

    /* Motor pins */
    ledc_channel_config_t ledc_channel_motor1 = {
        .speed_mode     = MOTOR_SPEED_MODE,
        .channel        = MOTOR_CHANNEL1,
        .timer_sel      = MOTOR_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = MOTOR_OUTPUT_DIR1,
        .duty           = MOTOR_PWM_MAX,
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_motor1));

    ledc_channel_config_t ledc_channel_motor2 = {
        .speed_mode     = MOTOR_SPEED_MODE,
        .channel        = MOTOR_CHANNEL2,
        .timer_sel      = MOTOR_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = MOTOR_OUTPUT_DIR2,
        .duty           = MOTOR_PWM_MAX,
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_motor2));

    /* PWM out pins */
    for (int i = 0 ; i < OUT_PWM_PINS ; ++i) {
        ledc_channel_config_t ledc_channel = {
            .speed_mode     = OUT_SPEED_MODE,
            .channel        = pwm_pin_channels[i],
            .timer_sel      = OUT_TIMER,
            .intr_type      = LEDC_INTR_DISABLE,
            .gpio_num       = pwm_pins[i],
            .duty           = 0,
            .hpoint         = 0
        };
        ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
    }

    /* Initialize other pins for motor driver */
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_DISABLE,
        .pin_bit_mask = 1ULL << MOTOR_INPUT_V,
        .pull_down_en = 0,
        .pull_up_en = 0,
    };
    gpio_config(&io_conf);

    /* Other GPIO pins */
    gpio_config_t io_conf_outputs = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << PHYS_OUTPUT_SMOKE),
        .pull_down_en = 0,
        .pull_up_en = 0,
    };
    gpio_config(&io_conf_outputs);

    ledc_fade_func_install(0);

    /* Main task for controlling speed */
    xTaskCreatePinnedToCore(engine_task, "engine_task", 2560, NULL, 5, NULL, 0);
}
