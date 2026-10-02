#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "string.h"

#include "cv.h"
#include "engine.h"
#include "vm.h"
#include "variables.h"
#include "pins.h"
#include "logger.h"
#include "drv8235.h"

#if CONFIG_BOARD_VERSION <= 2
#define MOTOR_SPEED_MODE        LEDC_LOW_SPEED_MODE
#define MOTOR_CHANNEL1          LEDC_CHANNEL_0
#define MOTOR_CHANNEL2          LEDC_CHANNEL_1
#define MOTOR_TIMER             LEDC_TIMER_0
#define MOTOR_PWM_FREQUENCY     40000
#define MOTOR_PWM_RESOLUTION    LEDC_TIMER_8_BIT
#define MOTOR_VOLTAGE_COUNT     10
#endif
#define MOTOR_PWM_MAX           255

typedef enum EngineState {
    ES_STOPPED = 0,
    ES_STARTING,
    ES_MOVING,
} EngineState;

static bool engine_should_stop;
static EngineState engine_state;
static uint8_t prev_speed;

#if CONFIG_BOARD_VERSION <= 2
static int accel_time;
#endif

static uint8_t engine_get_pwm(uint8_t speed)
{
    uint16_t min = cv_read(CV_VSTART);
    uint8_t max_speed = 255;
    if (vm_get_var(C_SWITCHING)) {
        max_speed = max_speed * cv_read(CV_SWITCHING_TRIM) / 128;
    }
    uint8_t range = min < max_speed ? max_speed - min : min;
    return min + (range * speed) / MOTOR_PWM_MAX;
}

bool engine_can_accelerate(void)
{
    return engine_state == ES_STOPPED
        || (engine_state == ES_MOVING
#if CONFIG_BOARD_VERSION >= 3
            //&& drv8235_get_speed() >= engine_get_pwm(engine_get_speed())
#endif
        );
}

void engine_hw_stop(void)
{
    engine_should_stop = true;
}


static void engine_task(void *args)
{
    while (true) {
#if CONFIG_BOARD_VERSION <= 2
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
                uint8_t pwm = MOTOR_PWM_MAX - cv_read(CV_KICK_START);
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
                    // Drive 0 output as PWM
                    uint8_t pwm = MOTOR_PWM_MAX - engine_get_pwm(speed);
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
            if (prev_speed != speed) {
                prev_speed = speed;
                /* Update speed */
                uint8_t s1 = MOTOR_PWM_MAX;
                uint8_t s2 = MOTOR_PWM_MAX;
                if (speed) {
                    bool dir = engine_get_direction();
                    // Drive 0 output as PWM
                    uint8_t pwm = MOTOR_PWM_MAX - engine_get_pwm(speed);
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
        }
#else
        uint8_t speed = engine_get_speed();
        if (engine_state == ES_MOVING && (engine_should_stop || speed == 0)) {
            engine_should_stop = false;
            engine_state = ES_STOPPED;
            prev_speed = 0;
            // No need in braking, this may switch the rotation
            // drv8235_set_motor_state(DRV8235_MOTOR_BRAKE);
            drv8235_set_speed(0);
        } else if (prev_speed != speed) {
            prev_speed = speed;
            /* Update speed */
            bool dir = engine_get_direction();
            uint8_t pwm = engine_get_pwm(speed);
            drv8235_set_motor_state(dir ? DRV8235_MOTOR_FORWARD : DRV8235_MOTOR_REVERSE);
            drv8235_set_speed(pwm);
            engine_state = ES_MOVING;
        }
#endif

        /* Wait */
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

char *engine_get_status(void)
{
// #if CONFIG_BOARD_VERSION >= 3
//     uint8_t speed = drv8235_get_speed();
//     uint8_t fault_status = drv8235_read_reg(DRV8235_REG_FAULT);
//     uint8_t status1 = drv8235_read_reg(DRV8235_REG_REG_STATUS1);
//     uint8_t status2 = drv8235_read_reg(DRV8235_REG_REG_STATUS2);
//     uint8_t status3 = drv8235_read_reg(DRV8235_REG_REG_STATUS3);
//     uint8_t throttle = engine_get_speed();
//     char *s = malloc(256);
//     sprintf(s, "Motor status:\n throttle = %d\n speed = %d\n fault_status = 0x%x\n voltage = %d\n current = %d\n duty = %d\n",
//         throttle, speed, fault_status, status1, status2, status3);
//     return s;
// #else
    return strdup("");
// #endif
}

void engine_init(void)
{
#if CONFIG_BOARD_VERSION <= 2
    /* Setup PWM pins for motor driver */
    ledc_timer_config_t ledc_timer_motor = {
        .speed_mode       = MOTOR_SPEED_MODE,
        .duty_resolution  = MOTOR_PWM_RESOLUTION,
        .timer_num        = MOTOR_TIMER,
        .freq_hz          = MOTOR_PWM_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    LOGGER_ERROR_CHECK(ledc_timer_config(&ledc_timer_motor));

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
#else
    drv8235_init();
#endif

    /* Task for controlling speed */
    xTaskCreatePinnedToCore(engine_task, "engine_task", 2560, NULL, 5, NULL, 0);
}
