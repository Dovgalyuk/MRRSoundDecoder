#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

bool file_read_uint8(FILE *f, uint8_t *v);
bool file_read_uint16(FILE *f, uint16_t *v);
bool file_read_uint32(FILE *f, uint32_t *v);
bool file_read_string(FILE *f, char **s);

#endif
