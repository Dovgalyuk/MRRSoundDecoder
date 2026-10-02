/* SPIFFS filesystem example.
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_littlefs.h"
#include "esp_partition.h"
#include "esp_flash.h"
#include "esp_flash_spi_init.h"
#include "driver/gpio.h"
#include "driver/spi_common.h"

#include "pins.h"
#include "storage.h"
#include "logger.h"

#define PARTITION_LABEL "sound"

#define TAG "LittleFS"

static bool storage_try_littlefs(void)
{
    esp_vfs_littlefs_conf_t conf = {
        .base_path = CONFIG_MOUNT_POINT,
        .partition_label = PARTITION_LABEL,
        .format_if_mount_failed = true,
        .dont_mount = false,
    };
    esp_err_t ret = esp_vfs_littlefs_register(&conf);

    if (ret != ESP_OK) {
        logger_printf(TAG ": Can't register partition '%s' at '%s'",
            conf.partition_label, conf.base_path);
        return false;
    }

    size_t total = 0, used = 0;
    ret = esp_littlefs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        logger_printf(TAG ": Failed to get LittleFS partition information (%s)", esp_err_to_name(ret));
        ret = esp_littlefs_format(conf.partition_label);
        if (ret != ESP_OK) {
            logger_printf(TAG ": Can't format partition '%s'", conf.partition_label);
            return false;
        }
    } else {
        logger_printf(TAG ": Partition size: total: %d, used: %d", total, used);
    }
    return true;
}

void storage_register_external_partition(void)
{
#if CONFIG_BOARD_VERSION==1
    spi_bus_config_t bus_config =
    {
        .data0_io_num = FLASH_SPI_D0,
        .data1_io_num = FLASH_SPI_D1,
        .data2_io_num = FLASH_SPI_D2,
        .data3_io_num = FLASH_SPI_D3,
        .sclk_io_num = FLASH_SPI_CLK,
        .max_transfer_sz = SPI_MAX_DMA_LEN
    };
    if (spi_bus_initialize(SPI3_HOST, &bus_config, SPI_DMA_CH_AUTO) != ESP_OK) {
        logger_printf(TAG ": Can't initialize external SPI bus");
        return;
    }

    esp_flash_spi_device_config_t flash_config = {
        .host_id = SPI3_HOST,
        .cs_io_num = FLASH_SPI_CS,
        .io_mode = SPI_FLASH_QIO,
        .freq_mhz = 20,
        .clock_source = SPI_CLK_SRC_DEFAULT,
    };

    esp_flash_t* ext_flash;
    if (spi_bus_add_flash_device(&ext_flash, &flash_config) != ESP_OK) {
        logger_printf(TAG ": Can't add SPI flash device");
        return;
    }

    esp_err_t err = esp_flash_init(ext_flash);
    if (err != ESP_OK) {
        logger_printf(TAG ": Can't initialize external flash %s (%x)", esp_err_to_name(err), err);
        return;
    }

    uint32_t id;
    ESP_ERROR_CHECK(esp_flash_read_id(ext_flash, &id));
    logger_printf(TAG ": Initialized external flash, size=%" PRIu32 " KB, ID=0x%" PRIx32, ext_flash->size / 1024, id);

    esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_DATA,
        ESP_PARTITION_SUBTYPE_DATA_LITTLEFS, NULL);
    const esp_partition_t *part;
    if (it) {
        part = esp_partition_get(it);
        esp_partition_iterator_release(it);
    } else {
        logger_printf(TAG ": Adding external flash as a partition, label=\"%s\", size=%" PRIu32 " KB",
            PARTITION_LABEL, ext_flash->size / 1024);
        if (esp_partition_register_external(ext_flash, 0, ext_flash->size, PARTITION_LABEL,
            ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_LITTLEFS, &part) != ESP_OK) {
            return;
        }
    }
    /* Pointer is not needed, will format partition with LittleFS library */
#endif
}

void storage_init(void)
{
    if (!storage_try_littlefs()) {
        storage_register_external_partition();
        storage_try_littlefs();
    }
}
