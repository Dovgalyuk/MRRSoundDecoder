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
    uint8_t sp;
    uint8_t nextsp;
    bool flag;
    /* 16-bit to allow signed values */
    int16_t stack[SLOT_STACK_SIZE];
    uint8_t locals[VAR_LOCAL_SIZE];
    uint32_t nextstack[NEXT_STACK_SIZE];
} Slot;

void slot_init(Slot *slot, Schedule *schedule);
void slot_clear(Slot *slot);
/* Returns true if wait instruction was executed */
bool slot_step(Slot *slot);
void slot_set_var(Slot *slot, uint16_t addr, uint8_t val);
uint8_t slot_get_var(Slot *slot, uint16_t addr);

void slot_started_sound(Slot *slot);
void slot_finished_sound(Slot *slot);

#endif
