#ifndef PROJECT_H
#define PROJECT_H

#include <stdint.h>
#include <stdbool.h>

#define PROJECT_FILENAME CONFIG_MOUNT_POINT"/sound.prj"

#define PROJECT_MAGIC 0x4452524d

#define PROJECT_STANDARD   0

void project_open(void);
void project_close(void);
void project_stop(void);
void project_tick(uint32_t t);

/* Provide project information */
const char *project_get_name(void);
const char *project_get_function_key_name(uint8_t id);

#endif
