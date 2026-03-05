#ifndef ENGINE_H
#define ENGINE_H

#include <stdint.h>
#include "vm.h"

#define ENGINE_FUNCTIONS VM_SLOTS

void engine_init(void);
void engine_tick(uint32_t t);

void engine_set_throttle(int16_t v);
int16_t engine_get_speed(void);
void engine_set_function(uint8_t f, int16_t v);
uint8_t engine_get_function(uint8_t f);

#endif
