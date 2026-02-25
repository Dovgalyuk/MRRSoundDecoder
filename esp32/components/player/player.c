#include <inttypes.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//#include "freertos/ringbuf.h"
#include "freertos/semphr.h"
// #include "driver/dac_continuous.h"
#include "driver/gptimer.h"
#include "driver/dac_oneshot.h"
#include "esp_check.h"
#include "esp_log.h"

#include "player.h"
#include "audio.h"
#include "slot.h"

#define TAG "player"

#define SOUND_CHANNELS 4
#define QUEUE_SIZE 1024

typedef struct SoundChannel {
    Slot *slot;
    /* channel is acquired when file is not NULL */
    WaveFile *file;
    uint8_t priority;
} SoundChannel;

static dac_oneshot_handle_t chan0_handle;
static SoundChannel channels[SOUND_CHANNELS];
static uint16_t queue[QUEUE_SIZE];
static volatile uint16_t queue_head, queue_tail;

#if 0

#define RINGBUF_HIGHEST_WATER_LEVEL    (32 * 1024)
#define RINGBUF_PREFETCH_WATER_LEVEL   (20 * 1024)

static const char *TAG = "dac_audio";

static dac_continuous_handle_t dac_handle;
static RingbufHandle_t ringbuf;
static SemaphoreHandle_t write_semaphore;

// static void player_dac_task(void *args)
// {
//     uint8_t *data = NULL;
//     size_t item_size = 0;
//     const size_t item_size_upto = 240 * 6;
//     size_t bytes_written = 0;

//     while (1) {
//         if (pdTRUE == xSemaphoreTake(write_semaphore, portMAX_DELAY)) {
//             while (1) {
//                 item_size = 0;
//                 /* receive data from ringbuffer and write it to DAC DMA transmit buffer */
//                 data = (uint8_t *)xRingbufferReceiveUpTo(ringbuf, &item_size, (TickType_t)pdMS_TO_TICKS(20), item_size_upto);
//                 if (item_size == 0) {
//                     ESP_LOGI(TAG, "ringbuffer underflowed!");
//                     // s_dac_cb.ringbuffer_mode = RINGBUFFER_MODE_PREFETCHING;
//                     break;
//                 }

//                 dac_continuous_write(dac_handle, data, item_size, &bytes_written, -1);
//                 vRingbufferReturnItem(ringbuf, (void *)data);
//             }
//         }
//     }
// }

static void player_mixer_task(void *args)
{
    size_t item_size = 0;

    while (1) {
        uint8_t data[256];
        uint16_t size = 0;
        dac_continuous_write(dac_handle, data, size, &bytes_written, -1);

        // xRingbufferSend(ringbuf, (void *)data, size, (TickType_t)0);

        // vRingbufferGetInfo(ringbuf, NULL, NULL, NULL, NULL, &item_size);
        // if (item_size >= RINGBUF_PREFETCH_WATER_LEVEL) {
        //     if (pdFALSE == xSemaphoreGive(write_semaphore)) {
        //         ESP_LOGE(TAG, "semaphore give failed");
        //     }
        // }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

#endif

/* Timer interrupt service routine */
static bool IRAM_ATTR player_dac_write_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data)
{
    uint8_t volume = 0;
    if (queue_head != queue_tail) {
        volume = queue[queue_head];
        queue_head = (queue_head + 1) % QUEUE_SIZE;
    }
    dac_oneshot_output_voltage(chan0_handle, volume);
    return false;
}

static void player_clear_channel(SoundChannel *ch)
{
    if (!ch->file) {
        return;
    }
    wave_close(ch->file);
    ch->file = NULL;
    slot_finished_sound(ch->slot);
}

static void player_mixer_task(void *args)
{
    while (true) {
        uint16_t next = (queue_tail + 1) % QUEUE_SIZE;
        if (next == queue_head) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        bool found = false;
        int16_t s = 0;
        for (int i = 0 ; i < SOUND_CHANNELS ; ++i) {
            if (channels[i].file) {
                uint16_t v;
                if (wave_next_sample(channels[i].file, &v)) {
                    found = true;
                    s = v;
                } else {
                    player_clear_channel(&channels[i]);
                    vTaskDelay(pdMS_TO_TICKS(30));
                }
            }
        }
        if (found) {
            queue[next] = (s / 0x100) + 0x80;
            queue_tail = next;
        }
    }
}

void player_init(void)
{
    xTaskCreatePinnedToCore(player_mixer_task, "player_task", 2560, NULL, 5, NULL, 1);
#if 0
    ESP_LOGI(TAG, "DAC audio example start");
    ESP_LOGI(TAG, "--------------------------------------");

    dac_continuous_config_t cont_cfg = {
        .chan_mask = DAC_CHANNEL_MASK_CH0,
        .desc_num = 4,
        .buf_size = 2048,
        .freq_hz = CONFIG_AUDIO_SAMPLE_RATE,
        .offset = 0,
        .clk_src = DAC_DIGI_CLK_SRC_DEFAULT,
        /* Assume the data in buffer is 'A B C D E F'
         * DAC_CHANNEL_MODE_SIMUL:
         *      - channel 0: A B C D E F
         *      - channel 1: A B C D E F
         * DAC_CHANNEL_MODE_ALTER:
         *      - channel 0: A C E
         *      - channel 1: B D F
         */
        .chan_mode = DAC_CHANNEL_MODE_SIMUL,
    };
    /* Allocate continuous channels */
    ESP_ERROR_CHECK(dac_continuous_new_channels(&cont_cfg, &dac_handle));

    if ((write_semaphore = xSemaphoreCreateBinary()) == NULL) {
        ESP_LOGE(TAG, "Semaphore create failed");
        return;
    }
    if ((ringbuf = xRingbufferCreate(RINGBUF_HIGHEST_WATER_LEVEL, RINGBUF_TYPE_BYTEBUF)) == NULL) {
        ESP_LOGE(TAG, "ringbuffer create failed");
        return;
    }

    // dac_event_callbacks_t cbs = {
    //     .on_convert_done = dac_on_convert_done_callback,
    //     .on_stop = NULL,
    // };
    // /* Must register the callback if using asynchronous writing */
    // ESP_ERROR_CHECK(dac_continuous_register_event_callback(dac_handle, &cbs, NULL));

    /* Enable the continuous channels */
    ESP_ERROR_CHECK(dac_continuous_enable(dac_handle));
    ESP_LOGI(TAG, "DAC initialized success, DAC DMA is ready");

    // ????
    // ESP_ERROR_CHECK(dac_continuous_start_async_writing(dac_handle));

    //xTaskCreate(player_dac_task, "player_task", 1024, NULL, 5, NULL);
    xTaskCreate(player_mixer_task, "player_task", 1024, NULL, 5, NULL);
#endif

    gptimer_handle_t gptimer = NULL;
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = CONFIG_AUDIO_SAMPLE_RATE,
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));
    dac_oneshot_config_t dac0_cfg = {
        .chan_id = DAC_CHAN_0,
    };
    ESP_ERROR_CHECK(dac_oneshot_new_channel(&dac0_cfg, &chan0_handle));

    gptimer_alarm_config_t alarm_config = {
        .reload_count = 0,
        .alarm_count = 1,
        .flags.auto_reload_on_alarm = true,
    };
    gptimer_event_callbacks_t cbs = {
        .on_alarm = player_dac_write_cb,
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs, NULL));
    ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config));
    ESP_ERROR_CHECK(gptimer_enable(gptimer));
    ESP_ERROR_CHECK(gptimer_start(gptimer));

    // TODO: create ring buffer filled by mixer
}

void player_clear(void)
{
    for (int i = 0 ; i < SOUND_CHANNELS ; ++i) {
        player_clear_channel(&channels[i]);
    }
}

void player_abort_slot(Slot *slot)
{
    for (int i = 0 ; i < SOUND_CHANNELS ; ++i) {
        if (channels[i].slot == slot) {
            player_clear_channel(&channels[i]);
            break;
        }
    }
}

static SoundChannel *player_acquire_channel(Slot *slot, uint8_t priority)
{
    /* TODO: work with priorities */
    for (int i = 0 ; i < SOUND_CHANNELS ; ++i) {
        if (!channels[i].file) {
            channels[i].slot = slot;
            channels[i].priority = priority;
            return &channels[i];
        }
    }
    return NULL;
}

void play_slot_sound(Slot *slot, uint16_t id, uint8_t priority)
{
    ESP_LOGI(TAG, "PLAY %d", id);
    SoundChannel *ch = player_acquire_channel(slot, priority);
    if (!ch) {
        printf("No available slots\n");
        return;
    }
    ch->file = wave_open(id);
    if (!ch->file) {
        printf("Can't open wave file\n");
        return;
    }
    slot_started_sound(slot);
}



/*
        // Stuff with silence to start
        uint32_t zeros[32] __attribute__((aligned(4))) = {};
        while (_out->availableForWrite() > 32) {
            _out->write((uint8_t *)zeros, sizeof(zeros));
        }

*/


/**
    @brief Generate a single frame worth of stereo samples by combining all inputs
void generateOneFrame() {
    // Collect all the input leg buffers
    int16_t *leg[_input.size()];
    for (size_t i = 0; i < _input.size(); i++) {
        leg[i] = (int16_t *)_input[i]->getResampledBuffer();
    }

    // Sum them up with saturating arithmetic
    for (size_t i = 0; i < _outWords * 2; i++) {
        int32_t sum = 0;
        for (size_t j = 0; j < _input.size(); j++) {
            sum += leg[j][i];
        }
        if (sum > 32767) {
            sum = 32767;
        } else if (sum < -32767) {
            sum = -32767;
        }
        _outBuff[i] = (int16_t)sum;
    }
}
*/


