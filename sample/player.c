#include <stdio.h>
#include <stdlib.h>
#include "player.h"
#include "audio.h"
#include "slot.h"
#include "engine.h"

/*
    Dumps all the played files into the single WAV file.
    Does not support mixing of the sounds.
*/

static FILE *wav;
static uint32_t fullsize;

static const uint8_t header[44] = "RIFF\xff\xff\xff\x00WAVEfmt "
    "\x10\x00\x00\x00\x01\x00\x01\x00\x12\x7a\x00\x00\x24\xf4\x00\x00\x02\x00\x10\x00"
    "data\xff\xff\xff\x00";
#define BUFFER_SIZE 2000
static uint16_t buffer[BUFFER_SIZE];

void player_init(void)
{
    const char *filename = "tmp.wav";
    wav = fopen(filename, "wb");
    fwrite(header, sizeof(header), 1, wav);
    fseek(wav, 0x18, SEEK_SET);
    uint32_t samplerate = wave_get_samplerate();
    fwrite((uint8_t*)&samplerate, sizeof(samplerate), 1, wav);
    samplerate *= 2;
    fwrite((uint8_t*)&samplerate, sizeof(samplerate), 1, wav);
    fseek(wav, sizeof(header), SEEK_SET);
}

void player_clear_sample(void)
{
    fseek(wav, 4, SEEK_SET);
    uint32_t t = fullsize + sizeof(header) - 8;
    fwrite(&t, 4, 1, wav);
    fseek(wav, sizeof(header) - 4, SEEK_SET);
    fwrite(&fullsize, 4, 1, wav);
    fclose(wav);
}

void player_tick(uint32_t delay)
{
    uint16_t maxsize = wave_get_samplerate() * delay / 1000;
    if (maxsize > BUFFER_SIZE) {
        maxsize = BUFFER_SIZE;
    }
    uint16_t len = 0;
    mixer_fill_buffer(buffer, maxsize, &len);
    if (len) {
        fullsize += len * 2;
        fwrite(buffer, 2, len, wav);
    }
}
