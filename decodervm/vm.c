#include <assert.h>
#include <stdio.h>
#include "vm.h"
#include "slot.h"

#define INSTRUCTIONS_PER_TICK 100

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
    return memory[addr];
}

void vm_load_slot(uint8_t id, const Schedule *sch)
{
    assert(id < VM_SLOTS);
    slot_init(&slots[id], sch);
}

void vm_set_slot_var(uint8_t id, uint16_t addr, uint8_t val)
{
    assert(id < VM_SLOTS);
    slot_set_var(&slots[id], addr, val);
}

void vm_tick(void)
{
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
    /* Run instructions */
    for (int i = 0 ; i < VM_SLOTS ; ++i) {
        if (!slots[i].schedule) {
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
