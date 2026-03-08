#ifndef PROJECT_H
#define PROJECT_H

#include <stdint.h>
#include <stdbool.h>

#define PROJECT_FILENAME CONFIG_MOUNT_POINT"/sound.prj"

#define PROJECT_MAGIC 0x4452524d

#define PROJECT_STANDARD   0
#define PROJECT_STEAM1     1
#define PROJECT_STEAM2     2

#define PROJECT_FUNCTIONS  64

void project_open(void);
void project_close(void);

void project_set_function(uint8_t f, bool v);
bool project_get_function_status(uint8_t f);

/* Provide project information */
const char *project_get_name(void);
const char *project_get_function_name(uint8_t id);

#endif
