#ifndef ENGINE_H
#define ENGINE_H

#include <stdint.h>
#include <stdio.h>
#include "vm.h"

#define ENGINE_THROTTLE_STEPS 28
#define PHYSICAL_OUTPUTS      7

typedef struct OutputProps {
    uint8_t flag_var;
    uint8_t delay_on;
    uint8_t delay_off;
} OutputProps;

void engine_init(void);
void engine_tick(uint32_t t);

void engine_set_throttle(uint8_t v);
uint8_t engine_get_speed(void);
uint8_t engine_get_speed_step(void);
bool engine_get_direction(void);
void engine_set_direction(bool d);
void engine_stop(void);
void engine_brake(void);

const OutputProps *engine_get_output_props(uint8_t id);
bool engine_load_output_props(FILE *f);

#endif
