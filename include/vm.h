#ifndef VM_H
#define VM_H

#include <stdint.h>
#include <stdbool.h>

#define VM_SLOTS      34

#define VM_STANDARD   0
#define VM_STEAM1     1
#define VM_STEAM2     2

typedef struct Slot Slot;
typedef struct Schedule Schedule;

/* Setup required hardware features (e.g., timers) */
void vm_init(void);
/* Loads schedules and other stuff */
void vm_load(const char *name);
/* Provide VM information */
const char *vm_get_name(void);
const char *vm_get_slot_name(uint8_t id);

uint8_t vm_get_var(uint16_t addr);
void vm_set_var(uint16_t addr, uint8_t val);

void vm_set_slot_var(uint8_t id, uint16_t addr, uint8_t val);
uint8_t vm_get_slot_var(uint8_t id, uint16_t addr);

void vm_tick(uint32_t t);
bool vm_has_drivelock(void);

#endif
