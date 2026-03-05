#ifndef SCHEDULE_H
#define SCHEDULE_H

#include <stdint.h>

typedef struct Schedule {
    uint32_t start;
    uint32_t script_size;
    uint8_t volume;
    char *name;
    uint8_t script[];
} Schedule;

#endif
