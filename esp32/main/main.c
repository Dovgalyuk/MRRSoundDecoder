/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_log.h"
/* Components */
#include "wifi.h"
#include "web.h"
#include "sd.h"
/* Decoder VM */
#include "schedule.h"
#include "slot.h"
#include "variables.h"
#include "vm.h"
#include "player.h"
#include "audio.h"
#include "engine.h"

#define TAG "main"
#define MOUNT_POINT "/sdcard"

uint64_t clock_read_ms(void)
{
    return esp_timer_get_time() / 1000;
}

static void vm_task(void *args)
{
    uint64_t last_clock = clock_read_ms();
    while (true) {
        uint64_t cur_clock = clock_read_ms();
        uint32_t t = cur_clock - last_clock;
        engine_tick(t);
        vm_tick(t);
        last_clock = cur_clock;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void app_main(void)
{
    /* Print chip information */
    esp_chip_info_t chip_info;
    uint32_t flash_size;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU core(s), %s%s%s%s, ",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "",
           (chip_info.features & CHIP_FEATURE_BT) ? "BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "BLE" : "",
           (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");

    unsigned major_rev = chip_info.revision / 100;
    unsigned minor_rev = chip_info.revision % 100;
    printf("silicon revision v%d.%d, ", major_rev, minor_rev);
    if(esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
        printf("Get flash size failed");
        return;
    }

    printf("%" PRIu32 "MB %s flash\n", flash_size / (uint32_t)(1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("Free heap size: %" PRIu32 " bytes\n", esp_get_free_heap_size());

    wifi_init();
    web_init();
    sd_init(MOUNT_POINT);
    vm_init();
    vm_load(MOUNT_POINT"/sound.prj");
    wave_init(MOUNT_POINT"/sound.prj");
    player_init();
    engine_init();

    printf("Free heap size after init: %" PRIu32 " bytes\n", esp_get_free_heap_size());

    xTaskCreatePinnedToCore(vm_task, "vm_task", 2560, NULL, 5, NULL, 0);
}
