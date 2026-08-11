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
    player_clear_emulator();
    player_clear();
    return NULL;
}

static void command_loop(void)
{
    while (true) {
        usleep(50 * 1000);
        printf("Active functions:");
        for (int i = 0 ; i < VM_FUNCTION_KEYS ; ++i) {
            if (vm_get_function_key(i)) {
                printf(" F%d", i);
            }
        }
        printf("\nSpeed: %d Direction: %s\n", engine_get_speed(),
            engine_get_direction() ? "forward" : "backward");
        printf("cmd> ");
        char command[32];
        (void)fgets(command, sizeof(command) - 1, stdin);
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
        case 'd':
        case 'D':
            vm_queue_command(VM_CMD_SET_DIRECTION, !engine_get_direction(), 0);
            break;
        case 'h':
        case 'H':
        case '?':
            printf("Available commands:\n");
            printf("\tH - display this help\n");
            printf("\tQ - quit\n");
            printf("\tF<id> - switch function key on/off\n");
            printf("\tT<throttle> - set throttle level\n");
            printf("\tB - start braking\n");
            printf("\tD - change direction\n");
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

    pthread_t thread_id;
    int s = pthread_create(&thread_id, NULL, &main_thread, NULL);
    if (s != 0) {
        fprintf(stderr, "pthread_create error %d\n", s);
    }

    command_loop();

    quit_program = true;
    pthread_join(thread_id, NULL);
}
