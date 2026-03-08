#include <inttypes.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2s_pins.h"
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

#define TAG "player"

#define SOUND_CHANNELS 4

#define PDM_TX_CLK_IO           I2S_BCLK_IO1      // I2S PDM TX clock io number
#define PDM_TX_DOUT_IO          I2S_DOUT_IO1      // I2S PDM TX data out io number
#define PDM_TX_FREQ_HZ          CONFIG_AUDIO_SAMPLE_RATE

typedef struct SoundChannel {
    Slot *slot;
    /* channel is acquired when file is not NULL */
    WaveFile *file;
    uint8_t priority;
    uint8_t volume;
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

/////////////////// main

    // gptimer_handle_t gptimer = NULL;
    // gptimer_config_t timer_config = {
    //     .clk_src = GPTIMER_CLK_SRC_DEFAULT,
    //     .direction = GPTIMER_COUNT_UP,
    //     .resolution_hz = 1000000/*CONFIG_AUDIO_SAMPLE_RATE*/,
    // };
    // ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));
    // dac_oneshot_config_t dac0_cfg = {
    //     .chan_id = DAC_CHAN_0,
    // };
    // ESP_ERROR_CHECK(dac_oneshot_new_channel(&dac0_cfg, &chan0_handle));

    // gptimer_alarm_config_t alarm_config = {
    //     .reload_count = 0,
    //     .alarm_count = 1000000 / CONFIG_AUDIO_SAMPLE_RATE,
    //     .flags.auto_reload_on_alarm = true,
    // };
    // gptimer_event_callbacks_t cbs = {
    //     .on_alarm = player_dac_write_cb,
    // };
    // ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs, NULL));
    // ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config));
    // ESP_ERROR_CHECK(gptimer_enable(gptimer));
    // ESP_ERROR_CHECK(gptimer_start(gptimer));

    // TODO: create ring buffer filled by mixer


//#endif

static void player_clear_channel(SoundChannel *ch)
{
    if (!ch->file) {
        return;
    }
    wave_close(ch->file);
    slot_finished_sound(ch->slot);
    ch->file = NULL;
}

static void player_mixer_task(void *args)
{
#if CONFIG_PLAYER_DAC
    while (true) {
        uint16_t size = (QUEUE_SIZE + queue_tail - queue_head) % QUEUE_SIZE;
        if (size >= QUEUE_MIN) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        uint16_t count = QUEUE_MAX - size;
        while (count--) {
            bool found = false;
            uint16_t next = (queue_tail + 1) % QUEUE_SIZE;
            int32_t s = 0;
            for (int i = 0 ; i < SOUND_CHANNELS ; ++i) {
                if (channels[i].file) {
                    uint16_t v;
                    if (wave_next_sample(channels[i].file, &v)) {
                        found = true;
                        s += (int16_t)v;
                    } else {
                        player_clear_channel(&channels[i]);
                        /* Switch to other tasks to allow continuous playing
                           by starting next sample. */
                        count = 0;
                    }
                }
            }
            if (!found) {
                break;
            }
            if (s > 32767) {
                s = 32767;
            } else if (s < -32767) {
                s = -32767;
            }
            s /= 2;
            queue[next] = (s + 0x8000) / 0x100;
            queue_tail = next;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
#endif
#if CONFIG_PLAYER_I2S
    while (true) {
        uint16_t count = BUFFER_SIZE;
        uint16_t last = 0;
        for (last = 0 ; last < count ; ++last) {
            bool found = false;
            int32_t s = 0;
            for (int i = 0 ; i < SOUND_CHANNELS ; ++i) {
                if (channels[i].file) {
                    uint16_t v;
                    if (wave_next_sample(channels[i].file, &v)) {
                        found = true;
                        s += (int16_t)v * (int32_t)channels[i].volume;
                    } else {
                        player_clear_channel(&channels[i]);
                        /* Switch to other tasks to allow continuous playing
                           by starting next sample. */
                        vTaskDelay(pdMS_TO_TICKS(10));
                    }
                }
            }
            if (!found) {
                break;
            }
            /* Divide by 100% volume */
            s /= 128;
            /* Saturated overflow */
            if (s > 32767) {
                s = 32767;
            } else if (s < -32767) {
                s = -32767;
            }
            buffer[last] = s;
        }
        if (last) {
            /* TODO: check timeout and retry? */
            if (i2s_channel_write(tx_chan, buffer, last * sizeof(uint16_t), NULL, 1000) != ESP_OK) {
                printf("Write Task: i2s write failed\n");
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
#endif
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
            channels[i].volume = slot->schedule->volume;
            return &channels[i];
        }
    }
    return NULL;
}

void play_slot_sound(Slot *slot, uint16_t id, uint8_t priority)
{
    ESP_LOGI(TAG, "PLAY %d speed=%d", id, vm_get_var(V_SPEED));
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
    /* TODO: get volume from Wave */
    slot_started_sound(slot);
}
