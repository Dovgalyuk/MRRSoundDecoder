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
#include "project.h"
#include "cv.h"

void player_tick(uint32_t delay);
void player_clear_sample(void);

void engine_hw_stop(void)
{
}

bool engine_can_accelerate(void)
{
    return true;
}

int main(int argc, char **argv)
{
    srand(time(NULL));

    cv_init();
    project_open();
    player_init();
    vm_init();

    for (int i = 0 ; i < VM_FUNCTION_KEYS ; ++i) {
        const char *name = project_get_function_key_name(i);
        if (name) {
            printf("Func %d = %s\n", i, name);
        }
    }

    engine_set_throttle(ENGINE_THROTTLE_STEPS);

    /* Start playing */
    //vm_set_slot_var(1, F_FUNCTION, 1);
    //vm_set_slot_var(32, F_FUNCTION, 1);
    //vm_set_var(C_KEY8, 1);
    vm_set_var(C_KEY1, 1);
    //vm_set_var(C_SLOT2, 1);
    //vm_set_var(C_SLOT22, 1);

    for (int i = 0 ; i < 10000 ; ++i) {
        //printf("%d : %d\n", i, slot.pc);
        //slot_step(&slot);
        engine_tick(10);
        vm_tick(10);
        player_tick(10);
        printf("speed %d accel %d\n", vm_get_var(V_SPEED), vm_get_var(V_ACCEL));
    }
    player_clear();
    player_clear_sample();
}
