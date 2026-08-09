#include <stdio.h>
#include <stdint.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <pthread.h>
#include <unistd.h>
#include "audio.h"
#include "player.h"

#define DELAY 10
#define BUFFER_SIZE 2000
#define BUFFERS 4

static uint16_t buffers[BUFFERS][BUFFER_SIZE];
static uint16_t buf_len[BUFFERS];
static pa_simple *s;
static bool exitting;
static pthread_t mixer_tid, player_tid;

static void *mixer_thread(void *arg)
{
    while (!exitting) {
        for (int i = 0 ; i < BUFFERS ; ++i) {
            if (!buf_len[i]) {
                uint16_t len = 0;
                mixer_fill_buffer(buffers[i], BUFFER_SIZE, &len);
                buf_len[i] = len;
                break;
            }
        }
        usleep(DELAY * 1000);
    }
    return NULL;
}

static void *player_thread(void *arg)
{
    while (!exitting) {
        for (int i = 0 ; i < BUFFERS ; ++i) {
            if (buf_len[i]) {
                pa_simple_write(s, buffers[i], buf_len[i] * 2, NULL);
                buf_len[i] = 0;
            }
        }
        usleep(DELAY * 1000);
    }
    return NULL;
}

void player_init(void)
{
    pa_sample_spec ss;

    ss.format = PA_SAMPLE_S16LE;
    ss.channels = 1;
    ss.rate = wave_get_samplerate();

    int error = 0;
    s = pa_simple_new(NULL,             // Use the default server.
                    "Train",            // Our application's name.
                    PA_STREAM_PLAYBACK,
                    NULL,               // Use the default device.
                    "Music",            // Description of our stream.
                    &ss,                // Our sample format.
                    NULL,               // Use default channel map
                    NULL,               // Use default buffering attributes.
                    &error              // Ignore error code.
                    );
    if (!s) {
        fprintf(stderr, "pa_simple_new() failed: %s\n", pa_strerror(error));
        return;
    }

    pthread_create(&player_tid, NULL, &player_thread, NULL);
    pthread_create(&mixer_tid, NULL, &mixer_thread, NULL);
}

void player_clear_emulator(void)
{
    exitting = true;
    pthread_join(mixer_tid, NULL);
    pthread_join(player_tid, NULL);
    pa_simple_free(s);
}
