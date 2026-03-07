#ifndef VM_H
#define VM_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define VM_SLOTS        34
#define VM_SLOT_BRAKE   33

typedef struct Slot Slot;
typedef struct Schedule Schedule;

/* Setup required hardware features (e.g., timers) */
void vm_init(void);
void vm_clear(void);
bool vm_load_slot(FILE *f);

uint8_t vm_get_var(uint16_t addr);
void vm_set_var(uint16_t addr, uint8_t val);

void vm_set_slot_var(uint8_t id, uint16_t addr, uint8_t val);
uint8_t vm_get_slot_var(uint8_t id, uint16_t addr);

void vm_tick(uint32_t t);
bool vm_has_drivelock(void);
void vm_reset_trigger(void);

#endif
