#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "audio.h"
#include "utils.h"
#include "logger.h"

#define BUFFER_SIZE 4096

typedef struct WaveInfo {
    uint32_t offset;
    uint32_t length;
    uint16_t samplerate;
    uint8_t bits;
} WaveInfo;

typedef struct WaveFile {
    WaveInfo *info;
    uint32_t last_offset;
    uint32_t size;
    union {
        uint8_t buffer[BUFFER_SIZE];
        int16_t samples[BUFFER_SIZE / 2];
    };
    uint16_t first;
    uint16_t sample_count;
    uint16_t prev_sample;
} WaveFile;

static FILE *wavepack;
static uint16_t wavecount;
static WaveInfo *waves;
static uint32_t wave_samplerate;

WaveFile *wave_open(uint16_t num)
{
    if (num >= wavecount) {
        return NULL;
    }
    WaveInfo *info = &waves[num];
    if (!info->offset) {
        return NULL;
    }
    if ((info->bits != 8 && info->bits != 16)
        || (info->samplerate != wave_samplerate / 2 && info->samplerate != wave_samplerate)) {
        return NULL;
    }
    /* Create and fill WaveFile */
    WaveFile *w = calloc(1, sizeof(WaveFile));
    if (!w) {
        logger_printf("Wave: Not enough memory");
        return NULL;
    }
    w->info = info;
    w->last_offset = info->offset;
    w->size = info->length;
    w->sample_count = 0;
    w->prev_sample = 0;
    w->sample_count = 0;
    w->first = 0;
    return w;
}

void wave_close(WaveFile *w)
{
    free(w);
}

static void wave_fetch_sample(WaveFile *w)
{
    if (!w->info || w->first < w->sample_count) {
        return;
    }
    if (w->sample_count) {
        w->prev_sample = w->samples[w->sample_count - 1];
    }
    uint32_t maxbytes = BUFFER_SIZE;
    if (w->info->bits == 8) {
        maxbytes /= 2;
    }
    if (w->info->samplerate < wave_samplerate) {
        maxbytes /= 2;
    }
    if (maxbytes > w->size) {
        maxbytes = w->size;
    }
    fseek(wavepack, w->last_offset, SEEK_SET);
    size_t sz = fread(w->buffer, 1, maxbytes, wavepack);
    w->last_offset += sz;
    w->size -= sz;
    uint16_t count;
    if (w->info->bits == 8) {
        count = sz;
        for (int i = count - 1 ; i >= 0 ; --i) {
            uint16_t current = ((int16_t)w->buffer[i] - 0x80) * 0x100;
            w->samples[i] = current;
        }
    } else {
        count = sz / 2;
    }
    if (!count) {
        w->info = NULL;
        return;
    }
    if (w->info->samplerate < wave_samplerate) {
        for (int i = count - 1 ; i >= 0 ; --i) {
            w->samples[i * 2 + 1] = w->samples[i];
        }
        count *= 2;
        for (int i = count - 2 ; i > 0 ; i -= 2) {
            w->samples[i] = ((int32_t)(int16_t)w->samples[i - 1] + (int16_t)w->samples[i + 1]) / 2;
        }
        w->samples[0] = ((int32_t)(int16_t)w->prev_sample + (int16_t)w->samples[1]) / 2;
    }
    w->first = 0;
    w->sample_count = count;
}

bool wave_next_sample(WaveFile *w, int16_t *sample)
{
    wave_fetch_sample(w);
    if (!w->info) {
        return false;
    }
    *sample = w->samples[w->first++];
    return true;
}

uint32_t wave_get_length(WaveFile *w)
{
    if (!w || !w->info) {
        return 0;
    }
    uint32_t samples = w->info->length;
    if (w->info->bits == 16) {
        samples /= 2;
    }
    if (w->info->samplerate < wave_samplerate) {
        samples *= 2;
    }
    return samples;
}

void wave_init(const char *name, uint32_t samplerate)
{
    wavepack = fopen(name, "rb");
    wave_samplerate = samplerate;
}

uint32_t wave_get_samplerate(void)
{
    return wave_samplerate;
}

bool wave_load_info(FILE *f)
{
    uint16_t num;
    if (!file_read_uint16(f, &num)) {
        return false;
    }
    uint32_t length;
    if (!file_read_uint32(f, &length)) {
        return false;
    }
    uint16_t samplerate;
    if (!file_read_uint16(f, &samplerate)) {
        return false;
    }
    uint8_t bits;
    if (!file_read_uint8(f, &bits)) {
        return false;
    }
    uint32_t offset = ftell(f);
    if (fseek(f, length, SEEK_CUR)) {
        return false;
    }
    if (num >= wavecount) {
        void *new_waves = realloc(waves, (num + 1) * sizeof(WaveInfo));
        if (!new_waves) {
            return false;
        }
        waves = new_waves;
        wavecount = num + 1;
    }
    WaveInfo *w = &waves[num];
    w->bits = bits;
    w->length = length;
    w->offset = offset;
    w->samplerate = samplerate;
    //printf("Wave %d bits=%d samplerate=%d length=%x offset=%x\n", num, bits, samplerate, length, offset);
    return true;
}

void wave_clear(void)
{
    if (wavepack) {
        fclose(wavepack);
        wavepack = NULL;
    }
    wavecount = 0;
    free(waves);
    waves = NULL;
}
