#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "storage.h"

static const char *TAG = "sd";

// Pin assignments can be set in menuconfig
// You can also change the pin assignments here by changing the following 4 lines.
#define PIN_NUM_MISO  CONFIG_SD_PIN_MISO
#define PIN_NUM_MOSI  CONFIG_SD_PIN_MOSI
#define PIN_NUM_CLK   CONFIG_SD_PIN_CLK
#define PIN_NUM_CS    CONFIG_SD_PIN_CS

void storage_init(void)
{
    esp_err_t ret;

    // Options for mounting the filesystem.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = true,
        .max_files = 2,
        .allocation_unit_size = 8 * 1024,
    };
    sdmmc_card_t *card;
    ESP_LOGI(TAG, "Initializing SD card");

    ESP_LOGI(TAG, "Using SPI peripheral");

    // By default, SD card frequency is initialized to SDMMC_FREQ_DEFAULT (20MHz)
    // For setting a specific frequency, use host.max_freq_khz (range 400kHz - 20MHz for SDSPI)
    // Example: for fixed frequency of 10MHz, use host.max_freq_khz = 10000;
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };

    /* Enable pullup for faster operation */
    gpio_set_direction(PIN_NUM_CLK, GPIO_MODE_INPUT_OUTPUT_OD);
    gpio_set_direction(PIN_NUM_CS, GPIO_MODE_INPUT_OUTPUT_OD);
    gpio_set_direction(PIN_NUM_MOSI, GPIO_MODE_INPUT_OUTPUT_OD);
    gpio_set_direction(PIN_NUM_MISO, GPIO_MODE_INPUT_OUTPUT_OD);
    gpio_pullup_en(PIN_NUM_CLK);
    gpio_pullup_en(PIN_NUM_CS);
    gpio_pullup_en(PIN_NUM_MOSI);
    gpio_pullup_en(PIN_NUM_MISO);

    ret = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize bus.");
        return;
    }

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = host.slot;

    ESP_LOGI(TAG, "Mounting filesystem");
    ret = esp_vfs_fat_sdspi_mount(CONFIG_MOUNT_POINT, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. ");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        return;
    }
    ESP_LOGI(TAG, "Filesystem mounted");

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);
}
