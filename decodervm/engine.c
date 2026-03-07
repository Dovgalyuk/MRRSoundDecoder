#include "engine.h"
#include "vm.h"
#include "variables.h"
#include "clock.h"

#define TICK_DURATION (896 / 5)

static int16_t throttle;
static int16_t speed;
static bool brake;

static const int CV3 = 2;
static const int CV4 = 2;

void engine_set_throttle(int16_t v)
{
    if (v >= -255 && v <= 255) {
        throttle = v;
    }
}

int16_t engine_get_speed(void)
{
    return speed;
}

void engine_set_function(uint8_t f, int16_t v)
{
    if (f < VM_SLOTS) {
        vm_set_slot_var(f, F_FUNCTION, v);
    }
}

uint8_t engine_get_function(uint8_t f)
{
    if (f < VM_SLOTS) {
        return vm_get_slot_var(f, F_FUNCTION);
    }
    return 0;
}

void engine_stop(void)
{
    throttle = 0;
    speed = 0;
}

void engine_brake(void)
{
    throttle = 0;
    brake = true;
}

void engine_tick(uint32_t t)
{
    static uint32_t dt;
    dt += t;
    if (dt < TICK_DURATION) {
        return;
    }
    dt -= TICK_DURATION;
    int16_t accel = 0;
    int mul = brake ? 5 : 1;
    /* calculate new speed */
    if (throttle != speed) {
        if (throttle < speed) {
            speed -= CV4 * mul;
            accel = -CV4 * mul;
            if (speed < throttle) {
                speed = throttle;
            }
        } else {
            speed += CV3 * mul;
            accel = CV3 * mul;
            if (speed > throttle) {
                speed = throttle;
            }
        }
        if (speed == 0) {
            brake = false;
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
    /* TODO: figure out the difference */
    vm_set_var(F_BRAKE1, brake);
    vm_set_var(F_BRAKE2, brake);
    vm_set_var(F_BRAKE3, brake);
    vm_set_slot_var(VM_SLOT_BRAKE, F_FUNCTION, brake);
}
