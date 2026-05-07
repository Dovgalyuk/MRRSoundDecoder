#ifndef SLOT_H
#define SLOT_H

#include <stdint.h>
#include <stdbool.h>
#include "variables.h"

typedef struct Schedule Schedule;

#define SLOT_STACK_SIZE 8
#define NEXT_STACK_SIZE 8

typedef struct Slot {
    Schedule *schedule;
    uint32_t pc;
    uint8_t id;
    uint8_t sp;
    uint8_t nextsp;
    /* Number of current sample (0-1) */
    uint8_t subslot;
    bool flag;
    bool error;
    /* 32-bit to allow signed 8-bit values and 32-bit addresses */
    int32_t stack[SLOT_STACK_SIZE];
    uint8_t locals[VAR_LOCAL_SIZE];
    uint32_t nextstack[NEXT_STACK_SIZE];
} Slot;

void slot_init(Slot *slot, Schedule *schedule);
void slot_clear(Slot *slot);
void slot_reset(Slot *slot);
/* Returns true if wait instruction was executed */
void slot_step(Slot *slot);
void slot_set_var(Slot *slot, uint16_t addr, uint8_t val);
uint8_t slot_get_var(Slot *slot, uint16_t addr);

void slot_started_sound(Slot *slot, uint8_t subslot);
void slot_finished_sound(Slot *slot, uint8_t subslot);

#endif
