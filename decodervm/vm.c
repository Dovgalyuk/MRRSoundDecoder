#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "vm.h"
#include "slot.h"
#include "schedule.h"
#include "clock.h"

#define INSTRUCTIONS_PER_TICK 4

static uint8_t vm_type;
static Slot slots[VM_SLOTS];
static uint8_t memory[VAR_GLOBAL_SIZE];

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

void vm_load_slot(uint8_t id, const Schedule *sch)
{
    assert(id < VM_SLOTS);
    // printf("Loading slot %d with %d bytes\n", id, sch->script_size);
    slot_init(&slots[id], sch);
}

void vm_set_slot_var(uint8_t id, uint16_t addr, uint8_t val)
{
    assert(id < VM_SLOTS);
    slot_set_var(&slots[id], addr, val);
}

void vm_tick(uint32_t t)
{
    static uint32_t trigger_time;
    /* Set trigger */
    trigger_time += t;
    uint32_t period = (100 + 255 - vm_get_var(V_SPEED)) * 2;
    if (trigger_time >= period) {
        trigger_time = 0;
    }
    if (vm_get_var(V_SPEED) && trigger_time <= 80) {
        vm_set_var(F_TRIGGER, 1);
    } else {
        vm_set_var(F_TRIGGER, 0);
    }

    static uint32_t clock_time;
    clock_time += t;
    if (clock_time >= 256) {
        clock_time = 0;
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
    // for (int i = 0 ; i < VM_SLOTS ; ++i) {
    for (int i = 1 ; i < 2 ; ++i) {
        if (!slots[i].schedule) {
            continue;
        }
        /* TODO: Special condition for break slot */
        if (i == 33) {
            continue;
        }
        for (int j = 0 ; j < INSTRUCTIONS_PER_TICK ; ++j) {
            if (slot_step(&slots[i])) {
                break;
            }
        }
    }
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

void vm_load(const char *name)
{
    FILE *f = fopen(name, "rb");
    if (!f) {
        return;
    }
    if (fread(&vm_type, 1, 1, f) != 1) {
        goto ret;
    }
    uint8_t slot;
    while (fread(&slot, 1, 1, f) == 1) {
        if (slots[slot].schedule) {
            goto ret;
        }
        uint32_t init, length;
        if (fread(&init, 4, 1, f) != 1) {
            goto ret;
        }
        if (fread(&length, 4, 1, f) != 1) {
            goto ret;
        }
        Schedule *sch = malloc(sizeof(Schedule) + length);
        if (!sch) {
            goto ret;
        }
        sch->start = init;
        sch->script_size = length;
        if (fread(sch->script, 1, length, f) != length) {
            free(sch);
            goto ret;
        }
        vm_load_slot(slot, sch);
    }
ret:
    fclose(f);
}
