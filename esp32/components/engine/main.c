/*
CVs

acceleration speed
slowdown speed
max speed
speed step

CV 2    Start volt
CV 3	Acceleration Rate (full speed for 0.896*CV3 seconds)
CV 4	Deceleration Rate

Functions

F1 - start engine
F6 - shunting mode (change speed without delay)
F7 - half speed

SV1-SV5 - speed ranges for faster sound

*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "engine.h"
#include "vm.h"
#include "variables.h"

// static void engine_trigger_task(void *args)
// {
//     while (true) {
//         if (speed == 0) {
//             vm_set_var(F_TRIGGER, 0);
//             vTaskDelay(pdMS_TO_TICKS(100));
//         } else {
//             int delay = 1000 + (255 - abs(speed)) * 6;
//             vm_set_var(F_TRIGGER, 1);
//             vTaskDelay(pdMS_TO_TICKS(delay));
//             vm_set_var(F_TRIGGER, 0);
//             vTaskDelay(pdMS_TO_TICKS(delay));
//         }
//     }
// }

static void engine_task(void *args)
{
    while (true) {
        //engine_tick();
        /* update hardware parameters */
        vTaskDelay(pdMS_TO_TICKS(896));
    }
}

void engine_init(void)
{
    /* Higher priority task for trigger signal */
    //xTaskCreatePinnedToCore(engine_trigger_task, "engine_trigger_task", 2560, NULL, 5, NULL, 1);
    /* Main task for controlling speed */
    xTaskCreatePinnedToCore(engine_task, "engine_task", 2560, NULL, 5, NULL, 1);
}
