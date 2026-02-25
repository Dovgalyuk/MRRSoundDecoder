#ifndef VM_H
#define VM_H

#include <stdint.h>
#include <stdbool.h>

#define VM_SLOTS        64

typedef struct Slot Slot;
typedef struct Schedule Schedule;

/* Setup required hardware features (e.g., timers) */
void vm_init(void);

uint8_t vm_get_var(uint16_t addr);
void vm_set_var(uint16_t addr, uint8_t val);

void vm_load_slot(uint8_t id, const Schedule *sch);
void vm_set_slot_var(uint8_t id, uint16_t addr, uint8_t val);

void vm_tick(void);
bool vm_has_drivelock(void);

#endif
