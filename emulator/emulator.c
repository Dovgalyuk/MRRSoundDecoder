#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include "schedule.h"
#include "slot.h"
#include "variables.h"
#include "vm.h"
#include "player.h"
#include "audio.h"
#include "engine.h"
#include "project.h"
#include "cv.h"

static bool quit_program;

void player_clear_emulator(void);

void engine_hw_stop(void)
{
}

bool engine_can_accelerate(void)
{
    return true;
}

static void *main_thread(void *arg)
{
    player_init();
    while (!quit_program) {
        engine_tick(10);
        vm_tick(10);
        usleep(10 * 1000);
    }
    player_clear();
    player_clear_emulator();
    return NULL;
}

static void command_loop(void)
{
    while (true) {
        printf("cmd> ");
        char command[32];
        fgets(command, sizeof(command) - 1, stdin);
        switch (command[0]) {
        case 'q':
        case 'Q':
            return;
        case 'f':
        case 'F':
            {
                int k = atol(command + 1);
                if (k >= 0 && k < VM_FUNCTION_KEYS) {
                    vm_queue_command(VM_CMD_SET_FUNCTION_STATE, k,
                        !vm_get_function_key(k));
                }
            }
            break;
        case 't':
        case 'T':
            {
                int t = atol(command + 1);
                if (t >= 0 && t <= ENGINE_THROTTLE_STEPS) {
                    vm_queue_command(VM_CMD_SET_THROTTLE, t, 0);
                }
            }
            break;
        case 'b':
        case 'B':
            vm_queue_command(VM_CMD_BRAKE, 0, 0);
            break;
        case 'h':
        case 'H':
        case '?':
            printf("Available commands:\n");
            printf("\th - display this help\n");
            printf("\tq - quit\n");
            printf("\tf<id> - switch function key on/off\n");
            printf("\tt<throttle> - set throttle level\n");
            printf("\tb - start braking\n");
            break;
        }
    }
}

int main(int argc, char **argv)
{
    srand(time(NULL));

    cv_init();
    project_open();
    vm_init();

    for (int i = 0 ; i < VM_FUNCTION_KEYS ; ++i) {
        const char *name = project_get_function_key_name(i);
        if (name) {
            printf("Func %d = %s\n", i, name);
        }
    }

    /* The pthread_create() call stores the thread ID into
        corresponding element of tinfo[].  */
    pthread_t thread_id;
    int s = pthread_create(&thread_id, NULL, &main_thread, NULL);
    if (s != 0) {
        fprintf(stderr, "pthread_create error %d\n", s);
    }

    command_loop();
    /* Start playing */
    //vm_set_slot_var(1, F_FUNCTION, 1);
    //vm_set_slot_var(32, F_FUNCTION, 1);
    //vm_set_var(C_KEY8, 1);
    //vm_set_var(C_SLOT2, 1);
    //vm_set_var(C_SLOT22, 1);

    quit_program = true;
    pthread_join(thread_id, NULL);
}
