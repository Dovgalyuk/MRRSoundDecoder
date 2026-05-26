#include <stdlib.h>
#include "cv.h"
#include "utils.h"

typedef struct CVDesc {
    const uint8_t default_value;
    const char *name;
    const char *description;
    const uint8_t min;
    const uint8_t max;
} CVDesc;

static const CVDesc cv_desc[CV_MAX + 1] = {
    [CV_VSTART] = {20, "Start motor voltage", "Start motor voltage for forward movement 255=VCC", 0, 255},
    [CV_ACCELERATION] = {10, "Acceleration", "0 for acceleration without delay", 0, 255},
    [CV_DECELERATION] = {10, "Deceleration", "0 for slowing down without delay", 0, 255},
    [CV_CHUFF_PERIOD] = {120, "Chuff period", "Steam chuffs period at speed 1 in 10s of milliseconds", 30, 255},
    [CV_CHUFF_SPEEDUP] = {96, "Chuff speedup", "Chuff speedup factor", 0, 255},
    [CV_CHUFF_MIN_PERIOD] = {150, "Minimum chuff period", "Chuff period could not be less at highest speeds", 0, 255},
    [CV_REVERSE_VSTART] = {20, "Reverse start motor voltage", "Start motor voltage for reverse movement 255=VCC", 0, 255},
    [CV_BRAKE_ON_THRESHOLD] = {60, "Brake On", "Brake sound swithes on when the speed is smaller or equals than this value", 0, 255},
    [CV_BRAKE_OFF_THRESHOLD] = {7, "Brake Off", "Brake sound swithes off when the speed is smaller than this value", 0, 255},
    [CV_LOAD_OPTIONAL] = {0, "Optional load", "Divided by 128 is the factor that changes acceleration and deceleration", 0, 255},
    [CV_LOAD_PRIMARY] = {255, "Primary load", "Divided by 128 is the factor that changes acceleration and deceleration", 0, 255},
    [CV_SWITCHING_TRIM] = {64, "Switching trim", "Divided by 128 is the factor that changes the speed in switching mode", 0, 255},
    [CV_SPEED_TABLE1] = {1, "Speed table step 1", "1-255", 1, 255},
    [CV_SPEED_TABLE2] = {2, "Speed table step 2", "1-255", 1, 255},
    [CV_SPEED_TABLE3] = {4, "Speed table step 3", "1-255", 1, 255},
    [CV_SPEED_TABLE4] = {7, "Speed table step 4", "1-255", 1, 255},
    [CV_SPEED_TABLE5] = {10, "Speed table step 5", "1-255", 1, 255},
    [CV_SPEED_TABLE6] = {14, "Speed table step 6", "1-255", 1, 255},
    [CV_SPEED_TABLE7] = {18, "Speed table step 7", "1-255", 1, 255},
    [CV_SPEED_TABLE8] = {23, "Speed table step 8", "1-255", 1, 255},
    [CV_SPEED_TABLE9] = {28, "Speed table step 9", "1-255", 1, 255},
    [CV_SPEED_TABLE10] = {34, "Speed table step 10", "1-255", 1, 255},
    [CV_SPEED_TABLE11] = {40, "Speed table step 11", "1-255", 1, 255},
    [CV_SPEED_TABLE12] = {47, "Speed table step 12", "1-255", 1, 255},
    [CV_SPEED_TABLE13] = {54, "Speed table step 13", "1-255", 1, 255},
    [CV_SPEED_TABLE14] = {62, "Speed table step 14", "1-255", 1, 255},
    [CV_SPEED_TABLE15] = {70, "Speed table step 15", "1-255", 1, 255},
    [CV_SPEED_TABLE16] = {79, "Speed table step 16", "1-255", 1, 255},
    [CV_SPEED_TABLE17] = {88, "Speed table step 17", "1-255", 1, 255},
    [CV_SPEED_TABLE18] = {98, "Speed table step 18", "1-255", 1, 255},
    [CV_SPEED_TABLE19] = {108, "Speed table step 19", "1-255", 1, 255},
    [CV_SPEED_TABLE20] = {120, "Speed table step 20", "1-255", 1, 255},
    [CV_SPEED_TABLE21] = {133, "Speed table step 21", "1-255", 1, 255},
    [CV_SPEED_TABLE22] = {147, "Speed table step 22", "1-255", 1, 255},
    [CV_SPEED_TABLE23] = {162, "Speed table step 23", "1-255", 1, 255},
    [CV_SPEED_TABLE24] = {178, "Speed table step 24", "1-255", 1, 255},
    [CV_SPEED_TABLE25] = {195, "Speed table step 25", "1-255", 1, 255},
    [CV_SPEED_TABLE26] = {213, "Speed table step 26", "1-255", 1, 255},
    [CV_SPEED_TABLE27] = {233, "Speed table step 27", "1-255", 1, 255},
    [CV_SPEED_TABLE28] = {255, "Speed table step 28", "1-255", 1, 255},
    [CV_FUNC_DEFAULT0] = {0, "Functions 0-7", "Enabled by default F0=1 F1=2 F2=4 F3=8 F4=16 F5=32 F6=64 F7=128", 0, 255},
    [CV_FUNC_DEFAULT1] = {0, "Functions 8-15", "Enabled by default F8=1 F9=2 F10=4 F11=8 F12=16 F13=32 F14=64 F15=128", 0, 255},
    [CV_FUNC_DEFAULT2] = {0, "Functions 16-23", "Enabled by default F16=1 F17=2 F18=4 F19=8 F20=16 F21=32 F22=64 F23=128", 0, 255},
    [CV_FUNC_DEFAULT3] = {0, "Functions 24-31", "Enabled by default F24=1 F25=2 F26=4 F27=8 F28=16 F29=32 F30=64 F31=128", 0, 255},
};

/*
64 Brake sound threshold «Brake On»
If the actual loco speed step is smaller than or equals the value indicated
here, the brake sound is triggered. Compare chapter 13.4.
60


65 Brake sound threshold «Brake
If the actual loco speed step is smaller than the one indicated here (up to
Off»
255), the brake sound will be switched off again. Compare chapter 13.4.
7

Brake 1-3 Section 10.6
*/

static uint8_t cv[CV_MAX + 1];


void cv_init()
{
    for (int i = 0 ; i <= CV_MAX ; ++i) {
        cv_set(i, cv_desc[i].default_value);
    }
}

uint8_t cv_read(cv_addr_t id)
{
    if (id >= CV_MAX) {
        return 0;
    }
    return cv[id];
}

void cv_set(cv_addr_t id, uint8_t value)
{
    if (id >= CV_MAX || value < cv_desc[id].min || value > cv_desc[id].max) {
        return;
    }
    cv[id] = value;
}

const char *cv_name(cv_addr_t id)
{
    if (id >= CV_MAX) {
        return NULL;
    }
    return cv_desc[id].name;
}

const char *cv_description(cv_addr_t id)
{
    if (id >= CV_MAX) {
        return NULL;
    }
    return cv_desc[id].description;
}

bool cv_load(FILE *f)
{
    uint16_t count;
    if (!file_read_uint16(f, &count)) {
        return false;
    }
    while (count--) {
        cv_addr_t id;
        if (!file_read_uint16(f, &id)) {
            return false;
        }
        uint8_t value;
        if (!file_read_uint8(f, &value)) {
            return false;
        }
        cv_set(id, value);
    }
    return true;
}
