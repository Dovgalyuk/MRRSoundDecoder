#include "engine.h"
#include "vm.h"
#include "variables.h"
#include "cv.h"
#include "utils.h"

#define TICK_DURATION    (896 / ENGINE_THROTTLE_STEPS)

static uint8_t throttle_step;
static uint8_t speed_step;
/* True for forward */
static bool direction = true;
static OutputProps output_props[PHYS_OUTPUTS];

void engine_set_throttle(uint8_t v)
{
    if (v <= ENGINE_THROTTLE_STEPS) {
        throttle_step = v;
    }
}

uint8_t engine_get_speed(void)
{
    return speed_step
        ? cv_read(CV_SPEED_TABLE1 + speed_step - 1)
        : 0;
}

uint8_t engine_get_speed_step(void)
{
    return speed_step;
}

void engine_set_direction(bool d)
{
    if (speed_step == 0) {
        direction = d;
    }
}

bool engine_get_direction(void)
{
    return direction;
}

void engine_stop(void)
{
    throttle_step = 0;
    speed_step = 0;
}

void engine_brake(void)
{
    throttle_step = 0;
}

const OutputProps *engine_get_output_props(uint8_t id)
{
    if (id < PHYS_OUTPUTS) {
        return &output_props[id];
    }
    return NULL;
}

bool engine_load_output_props(FILE *f)
{
    uint8_t num;
    if (!file_read_uint8(f, &num)) {
        return false;
    }
    if (num >= PHYS_OUTPUTS) {
        return false;
    }
    if (!file_read_uint8(f, &output_props[num - 1].delay_on)) {
        return false;
    }
    if (!file_read_uint8(f, &output_props[num - 1].delay_off)) {
        return false;
    }
    return true;
}

static uint16_t engine_check_load(uint16_t cv)
{
    if (vm_get_var(F_LOAD2) && cv_read(CV_LOAD_PRIMARY)) {
        cv = (cv * cv_read(CV_LOAD_PRIMARY)) / 128;
    } else if (vm_get_var(F_LOAD1) && cv_read(CV_LOAD_OPTIONAL)) {
        cv = (cv * cv_read(CV_LOAD_OPTIONAL)) / 128;
    }
    return cv;
}

void engine_tick(uint32_t t)
{
    /* Update immediate variables */
    vm_set_var(F_REVERSE, !direction);
    vm_set_var(V_SPEED_REQUEST, throttle_step
        ? cv_read(CV_SPEED_TABLE1 + throttle_step - 1)
        : 0);

    /* Wait for speed update */
    static uint32_t dt;
    if (vm_get_var(C_DRIVELOCK)) {
        dt = 0;
        return;
    }
    dt += t;
    /* cv3*896 = milliseconds to reach full speed */
    /* cv3*32 = milliseconds = delay per step */
    if ((!cv_read(CV_DECELERATION) && throttle_step < speed_step)
        || (!cv_read(CV_ACCELERATION) && throttle_step > speed_step)
        || vm_get_var(C_DISABLE_ACCEL)) {
        dt = 0;
    } else if (dt < TICK_DURATION) {
        return;
    } else {
        dt -= TICK_DURATION;
    }
    /* calculate new speed */
    int16_t accel;
    if (throttle_step != speed_step) {
        /* Should work fast enough (280ms) for CV=0 */
        static int accel_tick;
        ++accel_tick;
        if (throttle_step < speed_step) {
            uint16_t cv = cv_read(CV_DECELERATION);
            cv = engine_check_load(cv);
            int16_t prev = cv_read(CV_SPEED_TABLE1 + speed_step - 1);
            int16_t next = speed_step > 1
                ? cv_read(CV_SPEED_TABLE1 + speed_step - 2)
                : 0;
            accel = next - prev;
            if (accel_tick >= cv) {
                --speed_step;
                accel_tick = 0;
            }
        } else {
            uint16_t cv = cv_read(CV_ACCELERATION);
            cv = engine_check_load(cv);
            int16_t prev = speed_step
                ? cv_read(CV_SPEED_TABLE1 + speed_step - 1)
                : 0;
            int16_t next = cv_read(CV_SPEED_TABLE1 + speed_step);
            accel = next - prev;
            if (accel_tick >= cv) {
                ++speed_step;
                accel_tick = 0;
            }
        }
    } else {
        accel = 0;
    }

    int16_t speed = engine_get_speed();

    /* Calculate brake conditions */
    bool brake = accel < 0
        && speed <= cv_read(CV_BRAKE_ON_THRESHOLD)
        && speed >= cv_read(CV_BRAKE_OFF_THRESHOLD)
        && !vm_get_var(F_DISABLE_BRAKE);
    vm_set_var(F_BRAKING, brake);

    /* update speed, reverse, and acceleration in VM */
    vm_set_var(V_ACCEL, accel);
    /* TODO: figure out the difference between speeds */
    vm_set_var(V_SPEED_CURRENT, speed);
    vm_set_var(V_SPEED, speed);
    vm_set_var(F_DRIVING, speed != 0);
}
