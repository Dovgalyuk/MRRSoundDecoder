#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>
#include <stdbool.h>

#define WAVE_SAMPLERATE 31250

typedef struct WaveFile WaveFile;

void wave_init(const char *name);
bool wave_load_info(FILE *f);
WaveFile *wave_open(uint16_t num);
void wave_close(WaveFile *w);
bool wave_next_sample(WaveFile *w, int16_t *sample);
/* Length of the wave in samples */
uint32_t wave_get_length(WaveFile *w);
void wave_clear(void);

#endif
