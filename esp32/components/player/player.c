#include <inttypes.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "pins.h"
#include "esp_check.h"
#include "esp_log.h"

#if CONFIG_PLAYER_DAC
#include "driver/gptimer.h"
#include "driver/dac_oneshot.h"
#endif
#if CONFIG_PLAYER_I2S
#include "driver/i2s_pdm.h"
#include "driver/gpio.h"
#endif

#include "player.h"
#include "audio.h"
#include "slot.h"
#include "vm.h"
#include "variables.h"
#include "schedule.h"
#include "logger.h"

#define TAG "PLAYER"

#define SOUND_CHANNELS 8

#define PDM_TX_CLK_IO           I2S_BCLK_IO1      // I2S PDM TX clock io number
#define PDM_TX_DOUT_IO          I2S_DOUT_IO1      // I2S PDM TX data out io number
#define PDM_TX_FREQ_HZ          CONFIG_AUDIO_SAMPLE_RATE

typedef struct SoundChannel {
    Slot *slot;
    uint8_t subslot;
    /* channel is acquired when file is not NULL */
    WaveFile *file;
    uint32_t delay;
    uint32_t volume_step;
    uint32_t volume_timer;
    uint16_t volume_slot;
    uint8_t priority;
    uint8_t volume_cur;
    bool aborted;
} SoundChannel;

static SoundChannel channels[SOUND_CHANNELS];

#if CONFIG_PLAYER_DAC
#define QUEUE_SIZE 2048
#define QUEUE_MAX 1900
#define QUEUE_MIN 900
static uint8_t queue[QUEUE_SIZE];
static dac_oneshot_handle_t chan0_handle;
static volatile uint16_t queue_head, queue_tail;
#endif
#if CONFIG_PLAYER_I2S
/* Limited by DMA buffer size=2046 */
#define BUFFER_SIZE 2000
static uint16_t buffer[BUFFER_SIZE];
i2s_chan_handle_t tx_chan;
#endif

#if CONFIG_PLAYER_DAC

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

#endif

static void player_clear_channel(SoundChannel *ch)
{
    if (!ch->file) {
        return;
    }
    wave_close(ch->file);
    if (!ch->aborted) {
        slot_finished_sound(ch->slot, ch->subslot);
    }
    ch->file = NULL;
    ch->aborted = false;
}

static void player_mixer_task(void *args)
{
    while (true) {
#if CONFIG_PLAYER_DAC
        uint16_t size = (QUEUE_SIZE + queue_tail - queue_head) % QUEUE_SIZE;
        if (size >= QUEUE_MIN) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        uint16_t count = QUEUE_MAX - size;
        while (count--) {
            uint16_t next = (queue_tail + 1) % QUEUE_SIZE;
#endif
#if CONFIG_PLAYER_I2S
        uint16_t count = BUFFER_SIZE;
        uint16_t last = 0;
        for (last = 0 ; last < count ; ++last) {
#endif
            bool found = false;
            /* Requires three volume multiplications */
            int64_t s = 0;
            for (int i = 0 ; i < SOUND_CHANNELS ; ++i) {
retry:
                if (channels[i].file) {
                    uint16_t v;
                    if (!channels[i].aborted && channels[i].delay) {
                        found = true;
                        --channels[i].delay;
                    } else if (channels[i].aborted || !wave_next_sample(channels[i].file, &v)) {
                        player_clear_channel(&channels[i]);
                        /* Switch to other tasks to allow continuous playing
                           by starting next sample. */
                        vTaskDelay(pdMS_TO_TICKS(10));
                        /* Try to get rid of empty sample between files */
                        goto retry;
                    } else {
                        found = true;
                        s += (int16_t)v
                            * (int64_t)channels[i].volume_slot
                            * (int64_t)channels[i].volume_cur;
                        if (channels[i].volume_step) {
                            if (!--channels[i].volume_timer) {
                                channels[i].volume_timer = channels[i].volume_step;
                                ++channels[i].volume_cur;
                            }
                        }
                    }
                }
            }
            if (!found) {
                break;
            }
            /* Divide by 100% volume */
            s /= 128 * 128 * 128;
            if (s > 32767) {
                s = 32767;
            } else if (s < -32767) {
                s = -32767;
            }
#if CONFIG_PLAYER_DAC
            queue[next] = (s + 0x8000) / 0x100;
            queue_tail = next;
#endif
#if CONFIG_PLAYER_I2S
            buffer[last] = s;
#endif
        }
#if CONFIG_PLAYER_I2S
        if (last) {
            /* TODO: check timeout and retry? */
            if (i2s_channel_write(tx_chan, buffer, last * sizeof(uint16_t), NULL, 1000) != ESP_OK) {
                printf("Write Task: i2s write failed\n");
            }
        } else
#endif
        {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

void player_init(void)
{
#if CONFIG_PLAYER_DAC
    /* DAC */
    gptimer_handle_t gptimer = NULL;
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000/*CONFIG_AUDIO_SAMPLE_RATE*/,
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));
    dac_oneshot_config_t dac0_cfg = {
        .chan_id = DAC_CHAN_0,
    };
    ESP_ERROR_CHECK(dac_oneshot_new_channel(&dac0_cfg, &chan0_handle));

    gptimer_alarm_config_t alarm_config = {
        .reload_count = 0,
        .alarm_count = 1000000 / CONFIG_AUDIO_SAMPLE_RATE,
        .flags.auto_reload_on_alarm = true,
    };
    gptimer_event_callbacks_t cbs = {
        .on_alarm = player_dac_write_cb,
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs, NULL));
    ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config));
    ESP_ERROR_CHECK(gptimer_enable(gptimer));
    ESP_ERROR_CHECK(gptimer_start(gptimer));
#endif
#if CONFIG_PLAYER_I2S
    /* I2S */
    i2s_chan_config_t tx_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    tx_chan_cfg.auto_clear = true;
    tx_chan_cfg.dma_desc_num = 4;
    tx_chan_cfg.dma_frame_num = BUFFER_SIZE;
    ESP_ERROR_CHECK(i2s_new_channel(&tx_chan_cfg, &tx_chan, NULL));

    /* Step 2: Setting the configurations of PDM TX mode and initialize the TX channel
     * The slot configuration and clock configuration can be generated by the macros
     * These two helper macros is defined in 'i2s_pdm.h' which can only be used in PDM TX mode.
     * They can help to specify the slot and clock configurations for initialization or re-configuring */
    i2s_pdm_tx_config_t pdm_tx_cfg = {
        .clk_cfg = I2S_PDM_TX_CLK_DEFAULT_CONFIG(PDM_TX_FREQ_HZ),
        /* The data bit-width of PDM mode is fixed to 16 */
        .slot_cfg = I2S_PDM_TX_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .clk = PDM_TX_CLK_IO,
            .dout = PDM_TX_DOUT_IO,
            .invert_flags = {
                .clk_inv = false,
            },
        },
    };
    ESP_ERROR_CHECK(i2s_channel_init_pdm_tx_mode(tx_chan, &pdm_tx_cfg));

    /* Step 3: Enable the tx channel before writing data */
    ESP_ERROR_CHECK(i2s_channel_enable(tx_chan));
#endif

    xTaskCreatePinnedToCore(player_mixer_task, "player_task", 2560, 0, 5, NULL, 1);
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

static SoundChannel *player_acquire_channel(Slot *slot, uint8_t subslot, uint8_t priority)
{
    /* TODO: work with priorities */
    for (int i = 0 ; i < SOUND_CHANNELS ; ++i) {
        if (!channels[i].file) {
            channels[i].slot = slot;
            channels[i].subslot = subslot;
            channels[i].priority = priority;
            channels[i].volume_slot = slot->schedule->volume;
            channels[i].aborted = false;
            return &channels[i];
        }
    }
    return NULL;
}

void play_slot_sound(Slot *slot, uint8_t subslot, uint16_t id, uint8_t priority,
                     uint8_t volmin, uint8_t volmax, uint8_t delay)
{
    logger_printf(TAG " PLAY %d in %d/%d", id, slot->id, subslot);
    SoundChannel *ch = player_acquire_channel(slot, subslot, priority);
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
    ch->volume_slot *= wave_get_volume(ch->file);
    ch->delay = (delay * CONFIG_AUDIO_SAMPLE_RATE) / 1000;
    slot_started_sound(slot, subslot);
}
