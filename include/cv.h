#ifndef CV_H
#define CV_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

/* Standard */
#define CV_VSTART               2
#define CV_ACCELERATION         3
#define CV_DECELERATION         4
#define CV_EMF_CUTOUT           10

/* Custom 47-64 */
#define CV_CHUFF_PERIOD         47
#define CV_CHUFF_SPEEDUP        48
#define CV_CHUFF_MIN_PERIOD     49
//#define CV_REVERSE_VSTART       50
#define CV_BRAKE_ON_THRESHOLD   51
#define CV_BRAKE_OFF_THRESHOLD  52
#define CV_LOAD_OPTIONAL        53
#define CV_LOAD_PRIMARY         54
#define CV_SWITCHING_TRIM       55
#define CV_KICK_START_TIME      56
// #define CV_SOUND_VOLUME         63

#define CV_KICK_START           65
#define CV_SPEED_TABLE1         67
#define CV_SPEED_TABLE2         68
#define CV_SPEED_TABLE3         69
#define CV_SPEED_TABLE4         70
#define CV_SPEED_TABLE5         71
#define CV_SPEED_TABLE6         72
#define CV_SPEED_TABLE7         73
#define CV_SPEED_TABLE8         74
#define CV_SPEED_TABLE9         75
#define CV_SPEED_TABLE10        76
#define CV_SPEED_TABLE11        77
#define CV_SPEED_TABLE12        78
#define CV_SPEED_TABLE13        79
#define CV_SPEED_TABLE14        80
#define CV_SPEED_TABLE15        81
#define CV_SPEED_TABLE16        82
#define CV_SPEED_TABLE17        83
#define CV_SPEED_TABLE18        84
#define CV_SPEED_TABLE19        85
#define CV_SPEED_TABLE20        86
#define CV_SPEED_TABLE21        87
#define CV_SPEED_TABLE22        88
#define CV_SPEED_TABLE23        89
#define CV_SPEED_TABLE24        90
#define CV_SPEED_TABLE25        91
#define CV_SPEED_TABLE26        92
#define CV_SPEED_TABLE27        93
#define CV_SPEED_TABLE28        94

/* Custom 112-256 */
#define CV_FUNC_DEFAULT0        112
#define CV_FUNC_DEFAULT1        113
#define CV_FUNC_DEFAULT2        114
#define CV_FUNC_DEFAULT3        115

#define CV_MAX                  255

typedef uint16_t cv_addr_t;

void cv_init(void);
uint8_t cv_read(cv_addr_t id);
/* Set CV in RAM */
void cv_set(cv_addr_t id, uint8_t value);
/* Set CV in RAM and EEPROM */
void cv_write(cv_addr_t id, uint8_t value);
const char *cv_name(cv_addr_t id);
const char *cv_description(cv_addr_t id);
bool cv_load(FILE *f);

#endif
