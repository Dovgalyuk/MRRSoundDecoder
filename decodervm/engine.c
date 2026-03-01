#include "engine.h"
#include "vm.h"
#include "variables.h"
#include "clock.h"

static int16_t throttle;
static int16_t speed;

static const int CV3 = 5;
static const int CV4 = 5;

void engine_set_throttle(int16_t v)
{
    if (v >= -255 && v <= 255) {
        throttle = v;
    }
}

void engine_tick(uint32_t t)
{
    static uint32_t dt;
    dt += t;
    if (dt < 896) {
        return;
    }
    dt = 0;
    int16_t accel = 0;
    /* calculate new speed */
    if (throttle != speed) {
        if (throttle < speed) {
            speed -= CV4;
            accel = -CV4;
            if (speed < throttle) {
                speed = throttle;
            }
        } else {
            speed += CV3;
            accel = CV3;
            if (speed > throttle) {
                speed = throttle;
            }
        }
    }
    /* update speed, reverse, and acceleration in VM */
    vm_set_var(V_ACCEL, accel);
    if (speed >= 0) {
        /* TODO: figure out the difference between speeds */
        vm_set_var(V_SPEED_CURRENT, speed);
        vm_set_var(V_SPEED, speed);
        vm_set_var(F_REVERSE, 0);
    } else {
        vm_set_var(V_SPEED_CURRENT, -speed);
        vm_set_var(V_SPEED, -speed);
        vm_set_var(F_REVERSE, 1);
    }
    if (throttle >= 0) {
        vm_set_var(V_SPEED_REQUEST, throttle);
    } else {
        vm_set_var(V_SPEED_REQUEST, -throttle);
    }
}
