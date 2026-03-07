#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "vm.h"
#include "slot.h"
#include "schedule.h"
#include "clock.h"
#include "audio.h"
#include "utils.h"

#define INSTRUCTIONS_PER_TICK 100

static Slot slots[VM_SLOTS];
static uint8_t memory[VAR_GLOBAL_SIZE];
static bool trigger_set;

void vm_set_var(uint16_t addr, uint8_t val)
{
    addr -= VAR_GLOBAL_START;
    assert(addr < VAR_GLOBAL_SIZE);
    memory[addr] = val;
}

uint8_t vm_get_var(uint16_t addr)
{
    addr -= VAR_GLOBAL_START;
    assert(addr < VAR_GLOBAL_SIZE);
    uint8_t res = memory[addr];
    return res;
}

bool vm_load_slot(FILE *f)
{
    uint8_t slot;
    char *name = NULL;
    Schedule *sch = NULL;
    if (!file_read_uint8(f, &slot)) {
        goto error;
    }
    if (slot >= VM_SLOTS || slots[slot].schedule) {
        goto error;
    }
    if (!file_read_string(f, &name)) {
        goto error;
    }
    uint8_t volume;
    if (!file_read_uint8(f, &volume)) {
        goto error;
    }
    uint32_t init, length;
    if (!file_read_uint32(f, &init)) {
        goto error;
    }
    if (!file_read_uint32(f, &length)) {
        goto error;
    }
    sch = malloc(sizeof(Schedule) + length);
    if (!sch) {
        goto error;
    }
    sch->name = name;
    sch->volume = volume;
    sch->start = init;
    sch->script_size = length;
    if (fread(sch->script, 1, length, f) != length) {
        goto error;
    }
    //printf("Loading slot %d (%s) with %d bytes\n", slot, sch->name, sch->script_size);
    slot_init(&slots[slot], sch);
    return true;
error:
    free(name);
    free(sch);
    return false;
}

void vm_set_slot_var(uint8_t id, uint16_t addr, uint8_t val)
{
    assert(id < VM_SLOTS);
    slot_set_var(&slots[id], addr, val);
}

uint8_t vm_get_slot_var(uint8_t id, uint16_t addr)
{
    assert(id < VM_SLOTS);
    return slot_get_var(&slots[id], addr);
}

void vm_tick(uint32_t t)
{
    static uint32_t trigger_time;
    /* Set trigger */
    //trigger_time += t;
    // project mogul
    // uint32_t period = 950 - vm_get_var(V_SPEED) * 3;
    // project mogul2
    uint32_t period = 950 - vm_get_var(V_SPEED) * 3;
    period *= INSTRUCTIONS_PER_TICK / 10;

    static uint32_t clock_time;
    clock_time += t;
    while (clock_time >= 256) {
        clock_time -= 256;
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
        trigger_time += 1;
        if (trigger_time >= period) {
            trigger_time -= period;
            trigger_set = true;
        }
        if (vm_get_var(V_SPEED) && trigger_set) {
            vm_set_var(F_TRIGGER, 1);
        } else {
            vm_set_var(F_TRIGGER, 0);
        }

        for (int i = 0 ; i < VM_SLOTS ; ++i) {
        // for (int i = 0 ; i < 33 ; ++i) {
            if (!slots[i].schedule) {
                continue;
            }
            if (slot_step(&slots[i])) {
                break;
            }
        }
    }
}

void vm_reset_trigger(void)
{
    trigger_set = false;
}

bool vm_has_drivelock(void)
{
    for (int i = 0 ; i < VM_SLOTS ; ++i) {
        if (!slots[i].schedule) {
            continue;
        }
        if (slot_get_var(&slots[i], F_DRIVELOCK)) {
            return true;
        }
    }
    return false;
}

void vm_clear(void)
{
    for (int i = 0 ; i < VM_SLOTS ; ++i) {
        slot_clear(&slots[i]);
    }
}
