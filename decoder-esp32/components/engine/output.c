#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "driver/gpio.h"

#include "cv.h"
#include "output.h"
#include "vm.h"
#include "variables.h"
#include "pins.h"
#include "logger.h"

#define OUT_SPEED_MODE          LEDC_LOW_SPEED_MODE
#define OUT_PWM_FREQUENCY       40000
#define OUT_PWM_RESOLUTION      LEDC_TIMER_8_BIT
#define OUT_PWM_MAX             255
#define OUT_TIMER               LEDC_TIMER_1

#define OUT_PWM_PINS            6
#define LOGIC_PINS              1

typedef enum OutputState {
    OS_OFF = 0,
    OS_ON,
    OS_ON_TO_OFF,
    OS_OFF_TO_ON,
} OutputState;

static const uint8_t pwm_pins[OUT_PWM_PINS] = {
    PHYS_OUTPUT_FWD_LIGHT, PHYS_OUTPUT_BACK_LIGHT, PHYS_OUTPUT_4, PHYS_OUTPUT_5, PHYS_OUTPUT_6, PHYS_OUTPUT_7
};
static const uint8_t pwm_pin_channels[OUT_PWM_PINS] = {
    LEDC_CHANNEL_2, LEDC_CHANNEL_3, LEDC_CHANNEL_4, LEDC_CHANNEL_5, LEDC_CHANNEL_6, LEDC_CHANNEL_7
};
static const uint8_t logic_pins[LOGIC_PINS] = {
    PHYS_OUTPUT_SMOKE,
};

static OutputState pwm_pin_states[OUT_PWM_PINS];

static void output_task(void *args)
{
    while (true) {
        /* Update smoke and other logic pins */
        gpio_set_level(PHYS_OUTPUT_SMOKE, 0);

        /* Update LEDs */
        for (int i = 0 ; i < OUT_PWM_PINS ; ++i) {
            const OutputProps *p = output_get_props(i);
            bool cur = vm_get_var(p->flag_var);
            if (pwm_pin_states[i] == OS_OFF && cur) {
                uint32_t delay = p->delay_on;
                delay *= 1000;
                pwm_pin_states[i] = OS_OFF_TO_ON;
                LOGGER_ERROR_CHECK(ledc_set_fade_with_time(OUT_SPEED_MODE, pwm_pin_channels[i],
                                        OUT_PWM_MAX, delay));
                LOGGER_ERROR_CHECK(ledc_fade_start(OUT_SPEED_MODE, pwm_pin_channels[i],
                                LEDC_FADE_NO_WAIT));
            } else if (pwm_pin_states[i] == OS_ON && !cur) {
                uint32_t delay = p->delay_off;
                delay *= 1000;
                pwm_pin_states[i] = OS_OFF_TO_ON;
                ledc_set_fade_with_time(OUT_SPEED_MODE, pwm_pin_channels[i],
                                        0, delay);
                ledc_fade_start(OUT_SPEED_MODE, pwm_pin_channels[i],
                                LEDC_FADE_NO_WAIT);
            } else if (pwm_pin_states[i] == OS_ON) {
                uint8_t st = OUT_PWM_MAX;
                if (vm_get_var(C_DIMMER)) {
                    st = OUT_PWM_MAX * 4 / 10;
                }
                ledc_set_duty(OUT_SPEED_MODE, pwm_pin_channels[i], st);
                ledc_update_duty(OUT_SPEED_MODE, pwm_pin_channels[i]);
            }
        }

        /* Wait */
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

static IRAM_ATTR bool output_pwm_fade_end_event(const ledc_cb_param_t *param, void *user_arg)
{
    if (param->event == LEDC_FADE_END_EVT) {
        OutputState *s = user_arg;
        if (*s == OS_OFF_TO_ON) {
            *s = OS_ON;
        } else if (*s == OS_ON_TO_OFF) {
            *s = OS_OFF;
        }
    }
    return false;
}

void output_init(void)
{
    ledc_timer_config_t ledc_timer_out = {
        .speed_mode       = OUT_SPEED_MODE,
        .duty_resolution  = OUT_PWM_RESOLUTION,
        .timer_num        = OUT_TIMER,
        .freq_hz          = OUT_PWM_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    LOGGER_ERROR_CHECK(ledc_timer_config(&ledc_timer_out));

    /* PWM out pins */
    for (int i = 0 ; i < OUT_PWM_PINS ; ++i) {
        ledc_channel_config_t ledc_channel = {
            .speed_mode     = OUT_SPEED_MODE,
            .channel        = pwm_pin_channels[i],
            .timer_sel      = OUT_TIMER,
            //.intr_type      = LEDC_INTR_FADE_END,
            .gpio_num       = pwm_pins[i],
            .duty           = 0,
            .hpoint         = 0
        };
        LOGGER_ERROR_CHECK(ledc_channel_config(&ledc_channel));
    }

    /* Other GPIO pins */
    for (int i = 0 ; i < LOGIC_PINS ; ++i) {
        gpio_config_t io_conf_outputs = {
            .intr_type = GPIO_INTR_DISABLE,
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = (1ULL << logic_pins[i]),
            .pull_down_en = 0,
            .pull_up_en = 0,
        };
        LOGGER_ERROR_CHECK(gpio_config(&io_conf_outputs));
    }

    ledc_fade_func_install(0);

    ledc_cbs_t callbacks = {
        .fade_cb = output_pwm_fade_end_event
    };
    for (int i = 0 ; i < OUT_PWM_PINS ; ++i) {
        ledc_cb_register(OUT_SPEED_MODE, pwm_pin_channels[i],
            &callbacks, (void *)&pwm_pin_states[i]);
    }

    /* Task for outputs */
    xTaskCreatePinnedToCore(output_task, "output_task", 2560, NULL, 5, NULL, 0);
}
