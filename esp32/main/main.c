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

#define TAG "main"
#define MOUNT_POINT "/sdcard"

extern Schedule sch;

static void vm_task(void *args)
{
    while (true) {
        vm_tick();
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
    wave_init(MOUNT_POINT"/wave.pack");
    player_init();
    vm_init();

    printf("Free heap size after init: %" PRIu32 " bytes\n", esp_get_free_heap_size());

    //srand(time(NULL));

    //player_init(argv[1]);

    // vm_set_var(V_SPEED, 50);
    vm_set_var(V_SPEED_REQUEST, 50);
    // vm_set_var(V_ACCEL, 30);
    // vm_set_var(F_TRIGGER, 1);

    vm_load_slot(1, &sch);
    /* Start playing */
    vm_set_slot_var(1, F_FUNCTION, 1);

    xTaskCreatePinnedToCore(vm_task, "vm_task", 2560, NULL, 5, NULL, 1);
}
