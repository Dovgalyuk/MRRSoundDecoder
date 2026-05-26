#ifndef SCHEDULE_H
#define SCHEDULE_H

#include <stdint.h>

typedef struct Schedule {
    uint32_t script_size;
    uint16_t enable_var;
    uint8_t script[];
} Schedule;

#endif
