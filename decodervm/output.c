#include "output.h"
#include "variables.h"
#include "utils.h"

static OutputProps output_props[LOG_OUTPUTS];

const OutputProps *output_get_props(uint8_t id)
{
    if (id < LOG_OUTPUTS) {
        return &output_props[id];
    }
    return NULL;
}

bool output_load_props(FILE *f)
{
    uint8_t num;
    if (!file_read_uint8(f, &num)) {
        return false;
    }
    if (num >= LOG_OUTPUTS) {
        return false;
    }
    if (!file_read_uint8(f, &output_props[num - 1].flag_var)) {
        return false;
    }
    if (!file_read_uint8(f, &output_props[num - 1].delay_on)) {
        return false;
    }
    if (!file_read_uint8(f, &output_props[num - 1].delay_off)) {
        return false;
    }
    return true;
}

