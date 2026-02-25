#include <stdio.h>
#include <stdlib.h>
#include "player.h"
#include "audio.h"
#include "slot.h"

/*
    Dumps all the played files into the single WAV file.
    Does not support mixing of the sounds.
*/

static FILE *wav;
static uint32_t fullsize;

static const uint8_t header[44] = "RIFF\xff\xff\xff\x00WAVEfmt "
    "\x10\x00\x00\x00\x01\x00\x01\x00\x12\x7a\x00\x00\x24\xf4\x00\x00\x02\x00\x10\x00"
    "data\xff\xff\xff\x00";

void player_init(void)
{
    const char *filename = "tmp.wav";
    wav = fopen(filename, "wb");
    fwrite(header, sizeof(header), 1, wav);
}

void player_clear(void)
{
    fseek(wav, 4, SEEK_SET);
    uint32_t t = fullsize + sizeof(header) - 8;
    fwrite(&t, 4, 1, wav);
    fseek(wav, sizeof(header) - 4, SEEK_SET);
    fwrite(&fullsize, 4, 1, wav);
    fclose(wav);
}

void player_abort_slot(Slot *slot)
{
}

void play_slot_sound(Slot *slot, uint16_t id, uint8_t priority)
{
    printf("play %d\n", id);
    WaveFile *w = wave_open(id);
    if (!w) {
        printf("Can't open file %d\n", id);
        exit(1);
    }
    uint16_t sample;
    while (wave_next_sample(w, &sample)) {
        fullsize += 2;
        fwrite(&sample, sizeof(sample), 1, wav);
    }
    wave_close(w);
}
