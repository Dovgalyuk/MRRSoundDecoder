#include <stdlib.h>
#include "utils.h"

bool file_read_uint8(FILE *f, uint8_t *v)
{
    return fread(v, 1, 1, f) == 1;
}

bool file_read_uint16(FILE *f, uint16_t *v)
{
    return fread(v, 2, 1, f) == 1;
}

bool file_read_uint32(FILE *f, uint32_t *v)
{
    return fread(v, 4, 1, f) == 1;
}

bool file_read_string(FILE *f, char **s)
{
    uint16_t len;
    if (!file_read_uint16(f, &len)) {
        return false;
    }
    *s = malloc(len + 1);
    if (!*s) {
        return false;
    }
    if (fread(*s, 1, len, f) != len) {
        free(*s);
        *s = NULL;
        return false;
    }
    (*s)[len] = 0;
    return true;
}
