#include <inttypes.h>
#include <string.h>

#include "player.h"
#include "audio.h"
#include "slot.h"
#include "vm.h"
#include "variables.h"
#include "schedule.h"
#include "logger.h"

#define TAG "PLAYER"

#define SOUND_CHANNELS 8

typedef struct SoundChannel {
    Slot *slot;
    uint8_t subslot;
    /* channel is acquired when file is not NULL */
    WaveFile *file;
    uint32_t delay;
    uint32_t volume_step;
    uint32_t volume_timer;
    uint8_t volume_cur;
    bool aborted;
} SoundChannel;

static SoundChannel channels[SOUND_CHANNELS];
static bool player_on = true;


bool player_is_on(void)
{
    return player_on;
}

void player_set_onoff(bool onoff)
{
    player_on = onoff;
}

static void player_clear_channel(SoundChannel *ch)
{
    if (ch->file) {
        wave_close(ch->file);
    }
    if (!ch->aborted && ch->slot) {
        slot_finished_sound(ch->slot, ch->subslot);
    }
    ch->file = NULL;
    ch->delay = 0;
    ch->aborted = false;
    ch->slot = NULL;
}

void mixer_fill_buffer(uint16_t *buffer, uint16_t count, uint16_t *filled)
{
    uint16_t last = 0;
    bool processing = true;
    for (last = 0 ; processing && last < count ; ++last) {
        bool found = false;
        /* Requires three volume multiplications */
        int32_t s = 0;
        for (int i = 0 ; i < SOUND_CHANNELS ; ++i) {
            if (channels[i].file || channels[i].delay) {
                int16_t v;
                bool clear = false;
                if (channels[i].aborted) {
                    clear = true;
                } else if (channels[i].delay) {
                    if (!--channels[i].delay) {
                        clear = true;
                    } else {
                        found = true;
                    }
                /* TODO: next sample should report if it was the last one */
                } else if (!wave_next_sample(channels[i].file, &v)) {
                    clear = true;
                } else {
                    found = true;
                    s += v * channels[i].volume_cur;
                    if (channels[i].volume_step) {
                        if (!--channels[i].volume_timer) {
                            channels[i].volume_timer = channels[i].volume_step;
                            ++channels[i].volume_cur;
                        }
                    }
                }
                if (clear) {
                    player_clear_channel(&channels[i]);
                    processing = false;
                }
            }
        }
        if (!found) {
            break;
        }
        /* Divide by 100% volume */
        s /= 128;
        if (s > 32767) {
            s = 32767;
        } else if (s < -32767) {
            s = -32767;
        }
        /* Don't skip all the sounds, because they affect the behavior */
        if (!player_on) {
            s = 0;
        }
        buffer[last] = s;
    }
    *filled = last;
}

void player_clear(void)
{
    for (int i = 0 ; i < SOUND_CHANNELS ; ++i) {
        player_clear_channel(&channels[i]);
    }
}

void player_abort_slot(Slot *slot, uint8_t subslot)
{
    for (int i = 0 ; i < SOUND_CHANNELS ; ++i) {
        if (channels[i].slot == slot && channels[i].subslot == subslot) {
            channels[i].aborted = true;
            break;
        }
    }
}

static SoundChannel *player_acquire_channel(Slot *slot, uint8_t subslot)
{
    /* TODO: work with priorities */
    for (int i = 0 ; i < SOUND_CHANNELS ; ++i) {
        if (!channels[i].file && !channels[i].delay && !channels[i].aborted) {
            channels[i].slot = slot;
            channels[i].subslot = subslot;
            return &channels[i];
        }
    }
    return NULL;
}

void play_slot_delay(Slot *slot, uint8_t subslot, uint8_t delay)
{
    logger_printf(TAG " DELAY %d in %d/%d", delay, slot->id, subslot);
    SoundChannel *ch = player_acquire_channel(slot, subslot);
    if (!ch) {
        logger_printf(TAG " No available slots");
        return;
    }
    ch->delay = (delay * wave_get_samplerate()) / 1000;
    slot_started_delay(slot, subslot);
}

void play_slot_sound(Slot *slot, uint8_t subslot, uint16_t id,
                     uint8_t volmin, uint8_t volmax)
{
    logger_printf(TAG " PLAY %d in %d/%d vol=%d:%d", id, slot->id, subslot, volmin, volmax);
    SoundChannel *ch = player_acquire_channel(slot, subslot);
    if (!ch) {
        logger_printf(TAG " No available slots");
        return;
    }
    ch->file = wave_open(id);
    if (!ch->file) {
        logger_printf(TAG " Can't open wave file %d in %d/%d", id, slot->id, subslot);
        return;
    }
    ch->volume_cur = volmin;
    if (volmax != volmin) {
        uint32_t samples = wave_get_length(ch->file);
        samples /= volmax - volmin;
        ch->volume_step = samples;
        ch->volume_timer = samples;
    } else {
        ch->volume_step = 0;
        ch->volume_timer = 0;
    }
    ch->delay = 0;
    slot_started_sound(slot, subslot);
}
