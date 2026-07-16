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

/*
Getting Kick Start Working
Kick Start is programmed using Configuration Variable 65, and its value is determined by experimentation. By changing the value in CV 65, you change the specified amount of Kick Start applied to the motor when the throttle transitions from stop to the first speed step.

Before starting to adjust CVs:
Determine the CVs the multifunction decoder supports, and investigate the presence of additional CVs the manufacturer may have included for motor control purposes
The locomotive in question should be well maintained, lubricated, well running, and operate it for a few minutes to warm up the motor and lubricate the mechanism.
Ensure your locomotive moves at speed step 1. Get your locomotive started then back it down to speed step 1. If it stops moving, adjust the speed table or dither features first.
Finding the magic number for Kick Start takes a little time. If you have speed step 1 set to keep the locomotive running, but it will not start moving on speed step 1, increase the value in Kick Start. If the loco abruptly starts, or jolts to life, then settles back down to speed step 1, lower Kick Start value. When the locomotive instantly starts to crawl at the speed step 1 speed when first entering speed step 1, you've found the perfect setting.
Kick start is usually used in conjunction with user-loadable Speed Table. It is suggested you setup your speed table before tinkering with dither or Kick Start.

*/

static const CVDesc cv_desc[CV_MAX + 1] = {
    [CV_VSTART] = {20, "Start motor voltage", "Start motor voltage for forward movement 255=VCC", 0, 255},
    [CV_ACCELERATION] = {10, "Acceleration", "0 for acceleration without delay", 0, 255},
    [CV_DECELERATION] = {10, "Deceleration", "0 for slowing down without delay", 0, 255},
    [CV_EMF_CUTOUT] = {5, "EMF Cutout", "Speed step above which the back EMF motor control cuts off", 1, 28},
    [CV_CHUFF_PERIOD] = {120, "Chuff period", "Steam chuffs period at speed 1 in 10s of milliseconds", 30, 255},
    [CV_CHUFF_SPEEDUP] = {96, "Chuff speedup", "Chuff speedup factor", 0, 255},
    [CV_CHUFF_MIN_PERIOD] = {150, "Minimum chuff period", "Chuff period could not be less at highest speeds", 0, 255},
    [CV_BRAKE_ON_THRESHOLD] = {60, "Brake On", "Brake sound swithes on when the speed is smaller or equals than this value", 0, 255},
    [CV_BRAKE_OFF_THRESHOLD] = {7, "Brake Off", "Brake sound swithes off when the speed is smaller than this value", 0, 255},
    [CV_LOAD_OPTIONAL] = {0, "Optional load", "Divided by 128 is the factor that changes acceleration and deceleration", 0, 255},
    [CV_LOAD_PRIMARY] = {255, "Primary load", "Divided by 128 is the factor that changes acceleration and deceleration", 0, 255},
    [CV_SWITCHING_TRIM] = {64, "Switching trim", "Divided by 128 is the factor that changes the speed in switching mode", 0, 255},
    [CV_KICK_START_TIME] = {3, "Kick start time", "Time for kick start voltage in 10ms.", 0, 255},
    [CV_KICK_START] = {90, "Kick start", "Extra Kick that will supplied to the motor when starting.", 0, 255},
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
    [CV_SOUND1] = {0, "Sound CV 1", "Project-defined sound parameter", 0, 255},
    [CV_SOUND2] = {0, "Sound CV 2", "Project-defined sound parameter", 0, 255},
    [CV_SOUND3] = {0, "Sound CV 3", "Project-defined sound parameter", 0, 255},
    [CV_SOUND4] = {0, "Sound CV 4", "Project-defined sound parameter", 0, 255},
    [CV_SOUND5] = {0, "Sound CV 5", "Project-defined sound parameter", 0, 255},
    [CV_SOUND6] = {0, "Sound CV 6", "Project-defined sound parameter", 0, 255},
    [CV_SOUND7] = {0, "Sound CV 7", "Project-defined sound parameter", 0, 255},
    [CV_SOUND8] = {0, "Sound CV 8", "Project-defined sound parameter", 0, 255},
    [CV_SOUND9] = {0, "Sound CV 9", "Project-defined sound parameter", 0, 255},
    [CV_SOUND10] = {0, "Sound CV 10", "Project-defined sound parameter", 0, 255},
    [CV_SOUND11] = {0, "Sound CV 11", "Project-defined sound parameter", 0, 255},
    [CV_SOUND12] = {0, "Sound CV 12", "Project-defined sound parameter", 0, 255},
    [CV_SOUND13] = {0, "Sound CV 13", "Project-defined sound parameter", 0, 255},
    [CV_SOUND14] = {0, "Sound CV 14", "Project-defined sound parameter", 0, 255},
    [CV_SOUND15] = {0, "Sound CV 15", "Project-defined sound parameter", 0, 255},
    [CV_SOUND16] = {0, "Sound CV 16", "Project-defined sound parameter", 0, 255},
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
