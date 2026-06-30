#ifndef LOGGER_H
#define LOGGER_H

void logger_printf(const char *fmt, ...);
void logger_get_logs(char *buffer, size_t bufsize);

#define LOGGER_ERROR_CHECK(x) do {                                              \
    int err = (x);                                                              \
    if (err) {                                                                  \
        logger_printf("Error %d at %s:%d (%s)", err, __FILE__, __LINE__, #x);   \
    }                                                                           \
} while (0)

#endif
