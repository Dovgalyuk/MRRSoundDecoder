#ifndef SCHEDULE_H
#define SCHEDULE_H

#include <stdint.h>

/* F_FUNCTION is always set to 1 */
#define SCHEDULE_FLAG_FORCE 0x01
/* F_FUNCTION is not used */
//#define SCHEDULE_FLAG_AUTO  0x02
/* Schedule for braking sound */
#define SCHEDULE_FLAG_BRAKE 0x04

typedef struct Schedule {
    uint32_t script_size;
    uint8_t flags;
    uint8_t script[];
} Schedule;

#endif
