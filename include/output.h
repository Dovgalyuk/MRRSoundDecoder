#ifndef OUTPUT_H
#define OUTPUT_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define PHYSICAL_OUTPUTS      7

typedef struct OutputProps {
    uint8_t flag_var;
    uint8_t delay_on;
    uint8_t delay_off;
} OutputProps;

void output_init(void);

const OutputProps *output_get_props(uint8_t id);
bool output_load_props(FILE *f);


#endif
