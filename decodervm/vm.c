#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "vm.h"
#include "slot.h"
#include "schedule.h"
#include "audio.h"
#include "utils.h"
#include "cv.h"
#include "engine.h"
#include "player.h"
#include "logger.h"

#define INSTRUCTIONS_PER_TICK 100
#define CMD_QUEUE_SIZE        (8 * 3)

static Slot slots[VM_SLOTS];
static uint8_t memory[VAR_GLOBAL_SIZE];
static bool trigger_set;
static uint16_t cmd_queue[CMD_QUEUE_SIZE];
static uint16_t cmd_queue_head, cmd_queue_tail;

void vm_set_var(uint16_t addr, uint8_t val)
{
    addr -= VAR_GLOBAL_START;
    if (addr < VAR_GLOBAL_SIZE) {
        memory[addr] = val;
    }
}

uint8_t vm_get_var(uint16_t addr)
{
    addr -= VAR_GLOBAL_START;
    if (addr >= VAR_GLOBAL_SIZE) {
        return 0;
    }
    return memory[addr];
}

bool vm_load_slot(FILE *f)
{
    uint8_t slot;
    Schedule *sch = NULL;
    if (!file_read_uint8(f, &slot)) {
        printf("error %d\n", __LINE__);
        goto error;
    }
    if (slot >= VM_SLOTS || slots[slot].schedule) {
        printf("error %d\n", __LINE__);
        goto error;
    }
    uint8_t enable;
    if (!file_read_uint8(f, &enable)) {
        printf("error %d\n", __LINE__);
        goto error;
    }
    uint32_t length;
    if (!file_read_uint32(f, &length)) {
        printf("error %d\n", __LINE__);
        goto error;
    }
    sch = malloc(sizeof(Schedule) + length);
    if (!sch) {
        logger_printf("Not enough memory while loading slot %d of size %"PRId32"", slot, length);
        goto error;
    }
    sch->enable_var = enable;
    sch->script_size = length;
    if (fread(sch->script, 1, length, f) != length) {
        logger_printf("Error: can't load script code of length %d", length);
        goto error;
    }
    slots[slot].id = slot;
    slot_init(&slots[slot], sch);
    return true;
error:
    free(sch);
    return false;
}

void vm_set_slot_var(uint8_t id, uint16_t addr, uint8_t val)
{
    if (id < VM_SLOTS) {
        slot_set_var(&slots[id], addr, val);
    }
}

uint8_t vm_get_slot_var(uint8_t id, uint16_t addr)
{
    if (id >= VM_SLOTS) {
        return 0;
    }
    return slot_get_var(&slots[id], addr);
}

static void vm_cmd_queue_push_value(uint16_t v)
{
    cmd_queue[cmd_queue_tail] = v;
    cmd_queue_tail = (cmd_queue_tail + 1) % CMD_QUEUE_SIZE;
}

static uint16_t vm_cmd_queue_pop_value(void)
{
    uint16_t r = cmd_queue[cmd_queue_head];
    cmd_queue[cmd_queue_head] = 0;
    cmd_queue_head = (cmd_queue_head + 1) % CMD_QUEUE_SIZE;
    return r;
}

static void vm_process_queue(void)
{
    while (cmd_queue_head != cmd_queue_tail) {
        uint16_t cmd = vm_cmd_queue_pop_value();
        uint16_t p1 = vm_cmd_queue_pop_value();
        uint16_t p2 = vm_cmd_queue_pop_value();
        switch (cmd) {
        case VM_CMD_NONE:
            /* Really do nothing */
            break;
        case VM_CMD_STOP:
            engine_stop();
            player_clear();
            vm_reset();
            break;
        case VM_CMD_BRAKE:
            engine_brake();
            break;
        case VM_CMD_SET_THROTTLE:
            engine_set_throttle(p1);
            break;
        case VM_CMD_SET_DIRECTION:
            engine_set_direction(p1);
            break;
        case VM_CMD_SET_FUNCTION_STATE:
            vm_set_function_key(p1, p2);
            break;
        default:
            logger_printf("Unknown VM command %d", cmd);
            break;
        }
    }
}

void vm_queue_command(VMCommand cmd, uint16_t param1, uint16_t param2)
{
    /* Don't bother on queue overflow yet */
    vm_cmd_queue_push_value(cmd);
    vm_cmd_queue_push_value(param1);
    vm_cmd_queue_push_value(param2);
}

void vm_tick(uint32_t t)
{
    vm_process_queue();

    static int32_t trigger_time;
    /* Set trigger */
    //trigger_time += t;
    // project mogul
    // uint32_t period = 950 - vm_get_var(V_SPEED) * 3;
    // project mogul2
    int32_t period = cv_read(CV_CHUFF_PERIOD) * 10
                     - (int32_t)vm_get_var(V_SPEED) * cv_read(CV_CHUFF_SPEEDUP) / 2;
    int32_t min = cv_read(CV_CHUFF_MIN_PERIOD);
    if (period < min) {
        /* For highest speed need not to be prototypical */
        period = min;
    }
    trigger_time += t;
    if (trigger_time >= period) {
        trigger_time -= period;
        if (engine_get_speed_step()) {
            trigger_set = true;
            /* Switch the cylinder */
            vm_set_var(V_CYLINDER, vm_get_var(V_CYLINDER) + 1);
        }
    }

    static uint32_t clock_time, clock_time_256;
    clock_time += t;
    clock_time_256 += t;
    while (clock_time >= 1000) {
        clock_time -= 1000;
        /* Decrement timers */
        for (int i = 0 ; i < VM_SLOTS ; ++i) {
            if (!slots[i].schedule) {
                continue;
            }
            uint8_t v = slot_get_var(&slots[i], V_TIMER_1S);
            if (v) {
                slot_set_var(&slots[i], V_TIMER_1S, v - 1);
            }
        }
    }
    while (clock_time_256 >= 256) {
        clock_time_256 -= 256;
        /* Decrement timers */
        for (int i = 0 ; i < VM_SLOTS ; ++i) {
            if (!slots[i].schedule) {
                continue;
            }
            uint8_t v = slot_get_var(&slots[i], V_TIMER_1_256MS);
            if (v) {
                slot_set_var(&slots[i], V_TIMER_1_256MS, v - 1);
            }
            v = slot_get_var(&slots[i], V_TIMER_2_256MS);
            if (v) {
                slot_set_var(&slots[i], V_TIMER_2_256MS, v - 1);
            }
        }
    }
    /* Run instructions */
    for (int j = 0 ; j < INSTRUCTIONS_PER_TICK ; ++j) {
        // trigger_time += 1;
        // if (trigger_time >= period) {
        //     trigger_time -= period;
        //     trigger_set = true;
        // }
        /* Trigger at any speed to allow correct emergency stop */
        vm_set_var(F_TRIGGER, trigger_set);

        for (int i = 0 ; i < VM_SLOTS ; ++i) {
            if (!slots[i].schedule) {
                continue;
            }
            if (!vm_get_var(slots[i].schedule->enable_var)) {
                continue;
            }

            slot_step(&slots[i]);
        }
    }
}

void vm_reset_trigger(void)
{
    trigger_set = false;
}

void vm_init(void)
{
    vm_reset();
}

void vm_clear(void)
{
    for (int i = 0 ; i < VM_SLOTS ; ++i) {
        slot_clear(&slots[i]);
    }
}

void vm_reset(void)
{
    /* Clear everything */
    trigger_set = false;
    memset(memory, 0, sizeof(memory));
    for (int i = 0 ; i < VM_SLOTS ; ++i) {
        slot_reset(&slots[i]);
    }
    /* Set defaults */
    vm_init_function_keys();
    vm_set_var(F_RANDOM, 1);
    vm_set_var(F_EXECUTING, 1);
}

void vm_set_function_key(uint8_t f, bool v)
{
    if (f < VM_FUNCTION_KEYS) {
        uint8_t k = vm_get_var(C_KEY0 + f);
        if (v) {
            k |= 0x80;
        } else {
            k &= 0x7f;
        }
        vm_set_var(C_KEY0 + f, k);
    }
}

bool vm_get_function_key(uint8_t f)
{
    if (f < VM_FUNCTION_KEYS) {
        return vm_get_var(C_KEY0 + f);
    }
    return false;
}

void vm_init_function_keys(void)
{
    for (int i = 0 ; i < 4 ; ++i) {
        uint8_t bits = cv_read(CV_FUNC_DEFAULT0 + i);
        for (int j = 0 ; j < 8 ; ++j, bits >>= 1) {
            vm_set_function_key(i * 8 + j, bits & 1);
        }
    }
}
