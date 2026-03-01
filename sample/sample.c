#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <memory.h>
#include <time.h>
#include "schedule.h"
#include "slot.h"
#include "variables.h"
#include "vm.h"
#include "player.h"
#include "audio.h"
#include "engine.h"
#include "clock.h"

extern Schedule sch;

int main(int argc, char **argv)
{
    srand(time(NULL));

    wave_init("wave.pkg");
    vm_load("sound.prj");
    player_init();

    engine_set_throttle(255);

    /* Start playing */
    vm_set_slot_var(1, F_FUNCTION, 1);
    vm_set_var(V_SV_1, 32);
    vm_set_var(V_SV_2, 64);
    vm_set_var(V_SV_3, 128);
    vm_set_var(V_SV_4, 192);
    vm_set_var(V_SV_5, 250);
    //vm_set_slot_var(4, F_FUNCTION, 1);

    for (int i = 0 ; i < 10000 ; ++i) {
        //printf("%d : %d\n", i, slot.pc);
        //slot_step(&slot);
        engine_tick(10);
        vm_tick(10);
        printf("speed %d accel %d\n", vm_get_var(V_SPEED), vm_get_var(V_ACCEL));
    }
    player_clear();
}
