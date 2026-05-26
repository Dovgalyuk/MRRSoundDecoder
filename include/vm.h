#ifndef VM_H
#define VM_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define VM_SLOTS         128
#define VM_FUNCTION_KEYS 32

typedef struct Slot Slot;
typedef struct Schedule Schedule;

void vm_init(void);
void vm_clear(void);
void vm_reset(void);
bool vm_load_slot(FILE *f);

uint8_t vm_get_var(uint16_t addr);
void vm_set_var(uint16_t addr, uint8_t val);

void vm_set_slot_var(uint8_t id, uint16_t addr, uint8_t val);
uint8_t vm_get_slot_var(uint8_t id, uint16_t addr);

void vm_tick(uint32_t t);
void vm_reset_trigger(void);

void vm_set_function_key(uint8_t f, bool v);
bool vm_get_function_key(uint8_t f);
void vm_init_function_keys(void);

#endif
