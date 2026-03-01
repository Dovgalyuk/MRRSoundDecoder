#ifndef ENGINE_H
#define ENGINE_H

#include <stdint.h>

void engine_init(void);
void engine_set_throttle(int16_t v);
void engine_tick(uint32_t t);

#endif
