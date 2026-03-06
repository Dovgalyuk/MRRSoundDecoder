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

#include "storage.h"

static const char *TAG = "LittleFS";

void storage_init(void)
{
    esp_vfs_littlefs_conf_t conf = {
        .base_path = MOUNT_POINT,
        .partition_label = "sound",
        .format_if_mount_failed = true,
        .dont_mount = false,
    };
    esp_err_t ret = esp_vfs_littlefs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find LittleFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize LittleFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_littlefs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get LittleFS partition information (%s)", esp_err_to_name(ret));
        esp_littlefs_format(conf.partition_label);
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }
    // ESP_LOGI(TAG, "Initializing SPIFFS");

    // esp_vfs_spiffs_conf_t conf = {
    //   .base_path = MOUNT_POINT,
    //   .partition_label = NULL,
    //   .max_files = 2,
    //   .format_if_mount_failed = true
    // };

    // esp_err_t ret = esp_vfs_spiffs_register(&conf);

    // if (ret != ESP_OK) {
    //     if (ret == ESP_FAIL) {
    //         ESP_LOGE(TAG, "Failed to mount or format filesystem");
    //     } else if (ret == ESP_ERR_NOT_FOUND) {
    //         ESP_LOGE(TAG, "Failed to find SPIFFS partition");
    //     } else {
    //         ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
    //     }
    //     return;
    // }
    // size_t total = 0, used = 0;
    // ret = esp_spiffs_info(conf.partition_label, &total, &used);
    // if (ret != ESP_OK) {
    //     ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s). Formatting...", esp_err_to_name(ret));
    //     esp_spiffs_format(conf.partition_label);
    //     return;
    // } else {
    //     ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    // }
}
