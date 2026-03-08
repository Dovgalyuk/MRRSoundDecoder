/*
CVs

acceleration speed
slowdown speed
max speed
speed step

CV 2    Start volt
CV 3	Acceleration Rate (full speed for 0.896*CV3 seconds)
CV 4	Deceleration Rate

*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "driver/gpio.h"

#include "engine.h"
#include "vm.h"
#include "variables.h"

#define MOTOR_SPEED_MODE     LEDC_LOW_SPEED_MODE
#define MOTOR_OUTPUT_PWM     5
#define MOTOR_OUTPUT_DIR1    6
#define MOTOR_OUTPUT_DIR2    7
#define MOTOR_LEDC_CHANNEL   LEDC_CHANNEL_0
#define MOTOR_PWM_FREQUENCY  40000

static void motor_set_pwm(uint8_t v)
{
    ledc_set_duty(MOTOR_SPEED_MODE, MOTOR_LEDC_CHANNEL, v);
    ledc_update_duty(MOTOR_SPEED_MODE, MOTOR_LEDC_CHANNEL);
}

static void engine_task(void *args)
{
    while (true) {
        int16_t speed = engine_get_speed();
        if (speed == 0)
        {
            gpio_set_level(MOTOR_OUTPUT_DIR1, 0);
            gpio_set_level(MOTOR_OUTPUT_DIR2, 0);
            motor_set_pwm(0);
        }
        else if (speed > 0)
        {
            gpio_set_level(MOTOR_OUTPUT_DIR1, 1);
            gpio_set_level(MOTOR_OUTPUT_DIR2, 0);
            motor_set_pwm(speed);
        }
        else
        {
            gpio_set_level(MOTOR_OUTPUT_DIR1, 0);
            gpio_set_level(MOTOR_OUTPUT_DIR2, 1);
            motor_set_pwm(-speed);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void engine_init(void)
{
    /* Setup PWM pin for motor driver */
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = MOTOR_SPEED_MODE,
        .duty_resolution  = LEDC_TIMER_8_BIT,
        .timer_num        = LEDC_TIMER_0,
        .freq_hz          = MOTOR_PWM_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
   ledc_channel_config_t ledc_channel = {
        .speed_mode     = MOTOR_SPEED_MODE,
        .channel        = MOTOR_LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = MOTOR_OUTPUT_PWM,
        .duty           = 0,
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    /* Initialize other pins for motor driver */
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << MOTOR_OUTPUT_DIR1) | (1ULL << MOTOR_OUTPUT_DIR2),
        .pull_down_en = 0,
        .pull_up_en = 0,
    };
    gpio_config(&io_conf);

    /* Main task for controlling speed */
    xTaskCreatePinnedToCore(engine_task, "engine_task", 2560, NULL, 5, NULL, 1);
}
