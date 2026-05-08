#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "project.h"
#include "vm.h"
#include "engine.h"
#include "player.h"
#include "audio.h"
#include "utils.h"
#include "variables.h"
#include "cv.h"
#include "logger.h"

#define SECTION_INFO            1
#define SECTION_SLOT            2
#define SECTION_WAVE            3
#define SECTION_FUNCTION        4
#define SECTION_CV              5
#define SECTION_FUNCTION_KEY    6
#define SECTION_PHYSICAL_OUTPUT 7

#define PROJECT_FUNCTIONS 96

#define FUNCTION_MAX_IN   4
#define FUNCTION_MAX_OUT  4

static char *function_key_names[VM_FUNCTION_KEYS];

typedef struct Function {
    // TODO: add counters
    uint8_t inputs[FUNCTION_MAX_IN];
    uint8_t not_inputs[FUNCTION_MAX_IN];
    uint8_t logic[FUNCTION_MAX_OUT];
    uint8_t slots[FUNCTION_MAX_OUT];
    uint8_t physical[FUNCTION_MAX_OUT];
} Function;

static uint8_t project_type;
static char *project_name;

static Function *functions;
static uint8_t function_count;

static bool function_load_array(FILE *f, uint8_t max, uint8_t *arr)
{
    uint8_t len;
    if (!file_read_uint8(f, &len)) {
        return false;
    }
    if (len > max) {
        return false;
    }
    while (len--) {
        if (!file_read_uint8(f, arr)) {
            return false;
        }
        ++arr;
    }
    return true;
}

static bool project_load_function(FILE *f)
{
    void *new_functions = realloc(functions, (function_count + 1) * sizeof(Function));
    if (!new_functions) {
        return false;
    }
    functions = new_functions;
    ++function_count;
    memset(&functions[function_count - 1], 0, sizeof(Function));
    if (!function_load_array(f, FUNCTION_MAX_IN, functions[function_count - 1].inputs)) {
        return false;
    }
    if (!function_load_array(f, FUNCTION_MAX_IN, functions[function_count - 1].not_inputs)) {
        return false;
    }
    if (!function_load_array(f, FUNCTION_MAX_OUT, functions[function_count - 1].logic)) {
        return false;
    }
    if (!function_load_array(f, FUNCTION_MAX_OUT, functions[function_count - 1].slots)) {
        return false;
    }
    if (!function_load_array(f, FUNCTION_MAX_OUT, functions[function_count - 1].physical)) {
        return false;
    }
    return true;
}

static bool project_load_function_key(FILE *f)
{
    uint8_t num;
    if (!file_read_uint8(f, &num)) {
        return false;
    }
    if (num >= VM_FUNCTION_KEYS || function_key_names[num]) {
        return false;
    }
    if (!file_read_string(f, &function_key_names[num])) {
        return false;
    }
    return true;
}

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
    if (version != 0x10) {
        goto ret;
    }
    uint8_t section;
    while (file_read_uint8(f, &section)) {
        if (section == SECTION_INFO) {
            if (!file_read_uint8(f, &project_type)) {
                goto ret;
            }
            if (!file_read_string(f, &project_name)) {
                goto ret;
            }
        } else if (section == SECTION_SLOT) {
            if (!vm_load_slot(f)) {
                logger_printf("Error: can't load slot");
                goto ret;
            }
        } else if (section == SECTION_WAVE) {
            if (!wave_load_info(f)) {
                logger_printf("Error: can't load wave");
                goto ret;
            }
        } else if (section == SECTION_FUNCTION) {
            if (!project_load_function(f)) {
                logger_printf("Error: can't load function");
                goto ret;
            }
        } else if (section == SECTION_CV) {
            if (!cv_load(f)) {
                logger_printf("Error: can't load CV");
                goto ret;
            }
        } else if (section == SECTION_FUNCTION_KEY) {
            if (!project_load_function_key(f)) {
                logger_printf("Error: can't load function key");
                goto ret;
            }
        } else if (section == SECTION_PHYSICAL_OUTPUT) {
            if (!engine_load_output_props(f)) {
                logger_printf("Error: can't load physical output");
                goto ret;
            }
        } else {
            logger_printf("Error: unknown project section %d", section);
            goto ret;
        }
    }
    fclose(f);
    wave_init(PROJECT_FILENAME);
    logger_printf("Successfully loaded the project");
    return;
ret:
    logger_printf("Can't load the project");
    fclose(f);
}

void project_close(void)
{
    for (int i = 0 ; i < VM_FUNCTION_KEYS ; ++i) {
        free(function_key_names[i]);
        function_key_names[i] = NULL;
    }
    free(functions);
    functions = NULL;
    function_count = 0;
    engine_stop();
    vm_clear();
    player_clear();
    wave_clear();
    free(project_name);
    project_name = NULL;
}

void project_tick(uint32_t t)
{
    /* Reset all functions */
    for (uint8_t f = 0 ; f < function_count ; ++f) {
        for (int i = 0 ; i < FUNCTION_MAX_OUT ; ++i) {
            if (functions[f].logic[i]) {
                vm_set_var(functions[f].logic[i], 0);
            }
            if (functions[f].slots[i]) {
                vm_set_slot_var(functions[f].slots[i], F_FUNCTION, 0);
            }
        }
    }
    /* Reset all outputs */
    bool out[PHYS_OUTPUTS] = {};
    /* Validate conditions */
    for (uint8_t f = 0 ; f < function_count ; ++f) {
        /* Check inputs */
        bool active = true;
        for (int i = 0 ; i < FUNCTION_MAX_IN ; ++i) {
            if (functions[f].inputs[i]) {
                active &= vm_get_var(functions[f].inputs[i]) != 0;
            }
            if (functions[f].not_inputs[i]) {
                active &= vm_get_var(functions[f].not_inputs[i]) == 0;
            }
        }
        /* Set output */
        if (active) {
            for (int i = 0 ; i < FUNCTION_MAX_OUT ; ++i) {
                if (functions[f].logic[i]) {
                    vm_set_var(functions[f].logic[i], active);
                }
                if (functions[f].slots[i]) {
                    vm_set_slot_var(functions[f].slots[i], F_FUNCTION, active);
                }
            }
            for (int i = 0 ; i < FUNCTION_MAX_OUT ; ++i) {
                if (functions[f].physical[i]) {
                    out[functions[f].physical[i] - 1] = true;
                }
            }
        }
    }
    for (int i = 0 ; i < PHYS_OUTPUTS ; ++i) {
        vm_set_var(PHYS_OUTPUT0 + i, out[i]);
    }
}

const char *project_get_name(void)
{
    return project_name;
}

const char *project_get_function_key_name(uint8_t id)
{
    if (id < VM_FUNCTION_KEYS) {
        return function_key_names[id];
    }
    return NULL;
}

void project_stop(void)
{
    engine_stop();
    vm_reset();
    player_clear();
    for (int f = 0 ; f < VM_FUNCTION_KEYS ; ++f) {
        vm_set_function_key(f, false);
    }
}
