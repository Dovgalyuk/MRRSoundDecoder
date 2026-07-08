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
#include "esp_adc/adc_oneshot.h"
#include "string.h"

#include "cv.h"
#include "engine.h"
#include "vm.h"
#include "variables.h"
#include "pins.h"
#include "logger.h"

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

#define OUT_PWM_PINS            6

#define MOTOR_VOLTAGE_COUNT     10
typedef enum EngineState {
    ES_STOPPED = 0,
    ES_STARTING,
    ES_MOVING,
    ES_ACCEL_BEFORE_START,
    ES_ACCEL_START,
    ES_ACCEL_WAIT,
} EngineState;

static const uint8_t pwm_pins[OUT_PWM_PINS] = {PHYS_OUTPUT_FWD_LIGHT, PHYS_OUTPUT_BACK_LIGHT, PHYS_OUTPUT_4, PHYS_OUTPUT_5, PHYS_OUTPUT_6, PHYS_OUTPUT_7};
static const uint8_t pwm_pin_channels[OUT_PWM_PINS] = {LEDC_CHANNEL_2, LEDC_CHANNEL_3, LEDC_CHANNEL_4, LEDC_CHANNEL_5, LEDC_CHANNEL_6, LEDC_CHANNEL_7};
static bool pwm_pin_states[OUT_PWM_PINS];
static adc_oneshot_unit_handle_t adc_handle;
static uint8_t prev_speed;
static EngineState engine_state;
static uint32_t motor_voltage_start, motor_voltage_cur;
static uint8_t motor_voltage_count;
static int accel_time;
static bool engine_should_stop;
static uint16_t start_voltage;

bool engine_can_accelerate(void)
{
    return engine_state == ES_MOVING || engine_state == ES_STOPPED;
}

void engine_hw_stop(void)
{
    engine_should_stop = true;
}

static void engine_task(void *args)
{
    while (true) {
        uint8_t speed = engine_get_speed();
        if (engine_should_stop || speed == 0) {
            engine_should_stop = false;
            ledc_set_duty(MOTOR_SPEED_MODE, MOTOR_CHANNEL1, MOTOR_PWM_MAX);
            ledc_set_duty(MOTOR_SPEED_MODE, MOTOR_CHANNEL2, MOTOR_PWM_MAX);
            ledc_update_duty(MOTOR_SPEED_MODE, MOTOR_CHANNEL1);
            ledc_update_duty(MOTOR_SPEED_MODE, MOTOR_CHANNEL2);
            engine_state = ES_STOPPED;
            prev_speed = 0;
        } else if (engine_state == ES_STOPPED) {
            if (speed > 0) {
                prev_speed = speed;
                bool dir = engine_get_direction();
                start_voltage = cv_read(CV_KICK_START);
                uint8_t pwm = MOTOR_PWM_MAX - start_voltage;
                uint8_t s1 = MOTOR_PWM_MAX;
                uint8_t s2 = MOTOR_PWM_MAX;
                if (dir) {
                    s2 = pwm;
                } else {
                    s1 = pwm;
                }
                ledc_set_duty(MOTOR_SPEED_MODE, MOTOR_CHANNEL1, s1);
                ledc_set_duty(MOTOR_SPEED_MODE, MOTOR_CHANNEL2, s2);
                ledc_update_duty(MOTOR_SPEED_MODE, MOTOR_CHANNEL1);
                ledc_update_duty(MOTOR_SPEED_MODE, MOTOR_CHANNEL2);
                accel_time = 0;
                engine_state = ES_STARTING;
            }
        } else if (engine_state == ES_STARTING) {
            /* Read motor current */
            ++accel_time;
            if (accel_time >= cv_read(CV_KICK_START_TIME)) {
                engine_state = ES_MOVING;

                /* Update speed */
                uint8_t s1 = MOTOR_PWM_MAX;
                uint8_t s2 = MOTOR_PWM_MAX;
                if (speed) {
                    bool dir = engine_get_direction();
                    start_voltage = cv_read(CV_VSTART);
                    uint16_t min = start_voltage;
                    uint8_t max_speed = 255;
                    if (vm_get_var(C_SWITCHING)) {
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
            }
        } else if (engine_state == ES_MOVING) {
            if (prev_speed < speed) {
                engine_state = ES_ACCEL_BEFORE_START;
                motor_voltage_start = 0;
                motor_voltage_count = 0;

                prev_speed = speed;
                /* Update speed */
                uint8_t s1 = MOTOR_PWM_MAX;
                uint8_t s2 = MOTOR_PWM_MAX;
                if (speed) {
                    bool dir = engine_get_direction();
                    uint16_t min = start_voltage;
                    uint8_t max_speed = 255;
                    if (vm_get_var(C_SWITCHING)) {
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
            }
        } else if (engine_state == ES_ACCEL_BEFORE_START) {
            /* Read motor current */
            int motor_voltage = 0;
            LOGGER_ERROR_CHECK(adc_oneshot_read(adc_handle, MOTOR_ADC_CHANNEL, &motor_voltage));
            motor_voltage_start += motor_voltage;
            ++motor_voltage_count;
            if (motor_voltage_count == MOTOR_VOLTAGE_COUNT) {
                logger_printf("Motor speed=%d accel max1=%d", prev_speed, motor_voltage_start);
                motor_voltage_count = 0;
                accel_time = 0;
                engine_state = ES_ACCEL_START;
            }
        } else if (engine_state == ES_ACCEL_START) {
            /* Read motor current */
            int motor_voltage = 0;
            LOGGER_ERROR_CHECK(adc_oneshot_read(adc_handle, MOTOR_ADC_CHANNEL, &motor_voltage));
            motor_voltage_cur += motor_voltage;
            ++motor_voltage_count;
            if (motor_voltage_count == MOTOR_VOLTAGE_COUNT) {
                ++accel_time;
                if (motor_voltage_cur < motor_voltage_start) {
                    logger_printf("Motor accel max2=%d time=%d", motor_voltage_cur, accel_time);
                    accel_time = 0;
                    engine_state = ES_ACCEL_WAIT;
                } else {
                    motor_voltage_start = motor_voltage_cur;
                }
                motor_voltage_cur = 0;
                motor_voltage_count = 0;
            }
        } else if (engine_state == ES_ACCEL_WAIT) {
            /* Read motor current */
            int motor_voltage = 0;
            LOGGER_ERROR_CHECK(adc_oneshot_read(adc_handle, MOTOR_ADC_CHANNEL, &motor_voltage));
            motor_voltage_cur += motor_voltage;
            ++motor_voltage_count;
            if (motor_voltage_count == MOTOR_VOLTAGE_COUNT) {
                ++accel_time;
                if (motor_voltage_cur * 10 < motor_voltage_start * 7
                    || accel_time > 10) {
                    engine_state = ES_MOVING;
                    logger_printf("Motor accel min=%d time=%d", prev_speed, motor_voltage_cur, accel_time);
                } else {
                    motor_voltage_count = 0;
                    motor_voltage_cur = 0;
                }
            }
        }

        /* Wait */
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

static void output_task(void *args)
{
    while (true) {
        /* Update smoke */
        gpio_set_level(PHYS_OUTPUT_SMOKE, 0);

        /* Update LEDs */
        for (int i = 0 ; i < OUT_PWM_PINS ; ++i) {
            const OutputProps *p = engine_get_output_props(i);
            bool cur = vm_get_var(p->flag_var);
            if (cur != pwm_pin_states[i]) {
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
    LOGGER_ERROR_CHECK(ledc_timer_config(&ledc_timer_motor));
    ledc_timer_config_t ledc_timer_out = {
        .speed_mode       = OUT_SPEED_MODE,
        .duty_resolution  = OUT_PWM_RESOLUTION,
        .timer_num        = OUT_TIMER,
        .freq_hz          = OUT_PWM_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    LOGGER_ERROR_CHECK(ledc_timer_config(&ledc_timer_out));

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
    LOGGER_ERROR_CHECK(ledc_channel_config(&ledc_channel_motor1));

    ledc_channel_config_t ledc_channel_motor2 = {
        .speed_mode     = MOTOR_SPEED_MODE,
        .channel        = MOTOR_CHANNEL2,
        .timer_sel      = MOTOR_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = MOTOR_OUTPUT_DIR2,
        .duty           = MOTOR_PWM_MAX,
        .hpoint         = 0
    };
    LOGGER_ERROR_CHECK(ledc_channel_config(&ledc_channel_motor2));

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
        LOGGER_ERROR_CHECK(ledc_channel_config(&ledc_channel));
    }

#if CONFIG_BOARD_VERSION==2
#if 0
    /* Initialize pin for motor current measurement */
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = 1ULL << MOTOR_INPUT_V,
        .pull_down_en = 0,
        .pull_up_en = 0,
    };
    gpio_config(&io_conf);
#endif
    adc_oneshot_unit_init_cfg_t adc_init_config = {
        .unit_id = MOTOR_ADC_UNIT,
    };
    LOGGER_ERROR_CHECK(adc_oneshot_new_unit(&adc_init_config, &adc_handle));

    adc_oneshot_chan_cfg_t adc_channel_config = {
        .atten = ADC_ATTEN_DB_0,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    LOGGER_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, MOTOR_ADC_CHANNEL, &adc_channel_config));
#endif

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
    /* Additional task for outputs */
    xTaskCreatePinnedToCore(output_task, "output_task", 2560, NULL, 5, NULL, 0);
}
