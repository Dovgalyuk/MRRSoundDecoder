#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "project.h"
#include "vm.h"
#include "engine.h"
#include "player.h"
#include "audio.h"
#include "utils.h"
#include "variables.h"

typedef struct Function {
    char *name;
    bool enabled;
    uint8_t slot_count;
    uint8_t *slots;
} Function;

static uint8_t project_type;
static char *project_name;
static Function functions[PROJECT_FUNCTIONS];

void project_open(void)
{
    FILE *f = fopen(PROJECT_FILENAME, "rb");
    if (!f) {
        return;
    }
    uint32_t magic;
    if (!file_read_uint32(f, &magic)) {
        goto ret;
    }
    if (magic != PROJECT_MAGIC) {
        goto ret;
    }
    uint8_t version;
    if (!file_read_uint8(f, &version)) {
        goto ret;
    }
    if (version != 1) {
        goto ret;
    }
    uint8_t record;
    while (file_read_uint8(f, &record)) {
        if (record == 1) {
            if (!file_read_uint8(f, &project_type)) {
                goto ret;
            }
            if (!file_read_string(f, &project_name)) {
                goto ret;
            }
        } else if (record == 2) {
            if (!vm_load_slot(f)) {
                goto ret;
            }
        } else if (record == 3) {
            if (!wave_load_info(f)) {
                goto ret;
            }
        } else if (record == 4) {
            uint8_t count;
            if (!file_read_uint8(f, &count)) {
                goto ret;
            }
            while (count--) {
                uint8_t num;
                if (!file_read_uint8(f, &num)) {
                    goto ret;
                }
                if (num >= PROJECT_FUNCTIONS || functions[num].name) {
                    goto ret;
                }
                if (!file_read_string(f, &functions[num].name)) {
                    goto ret;
                }
                if (!file_read_uint8(f, &functions[num].slot_count)) {
                    goto ret;
                }
                if (functions[num].slot_count) {
                    functions[num].slots = calloc(functions[num].slot_count, 1);
                    if (!functions[num].slots) {
                        goto ret;
                    }
                    for (int i = 0 ; i < functions[num].slot_count ; ++i) {
                        if (!file_read_uint8(f, &functions[num].slots[i])) {
                            goto ret;
                        }
                    }
                }
            }
        } else {
            goto ret;
        }
    }
    wave_init(PROJECT_FILENAME);
ret:
    fclose(f);
}

void project_close(void)
{
    for (int i = 0 ; i < PROJECT_FUNCTIONS ; ++i) {
        free(functions[i].name);
        free(functions[i].slots);
        memset(functions + i, 0, sizeof(Function));
    }
    engine_stop();
    vm_clear();
    player_clear();
    wave_clear();
    free(project_name);
    project_name = NULL;
}

void project_set_function(uint8_t f, bool v)
{
    assert(f < PROJECT_FUNCTIONS);
    functions[f].enabled = v;
    for (int i = 0 ; i < functions[f].slot_count ; ++i) {
        vm_set_slot_var(functions[f].slots[i], F_FUNCTION, v);
    }
}

bool project_get_function_status(uint8_t f)
{
    assert(f < PROJECT_FUNCTIONS);
    return functions[f].enabled;
}

const char *project_get_name(void)
{
    return project_name;
}

const char *project_get_function_name(uint8_t id)
{
    return functions[id].name;
}
