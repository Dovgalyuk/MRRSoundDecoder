#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "cv.h"

#define TAG "cv_storage"

#define STORAGE_NAMESPACE "CV"
#define KEY_LEN           5

static nvs_handle_t nvs;

static cv_addr_t cv_decode_key(const char *key)
{
    int res = 0;
    sscanf(key, "%x", &res);
    return res;
}

static void cv_encode_key(cv_addr_t id, char *key)
{
    sprintf(key, "%x", id);
}

void cv_write(cv_addr_t id, uint8_t value)
{
    char key[KEY_LEN + 1];
    cv_encode_key(id, key);
    ESP_ERROR_CHECK(nvs_set_u8(nvs, key, value));
    cv_set(id, value);
}

void cv_storage_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    if (nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvs) != ESP_OK) {
        return;
    }

    nvs_iterator_t it = NULL;
    esp_err_t ret = nvs_entry_find_in_handle(nvs, NVS_TYPE_U8, &it);
    while (ret == ESP_OK) {
        nvs_entry_info_t info;
        ret = nvs_entry_info(it, &info);
        if (ret != ESP_OK) {
            break;
        }

        uint8_t value;
        if (nvs_get_u8(nvs, info.key, &value) != ESP_OK) {
            break;
        }
        cv_addr_t cv = cv_decode_key(info.key);
        ESP_LOGI(TAG, "Loading CV %d = %d\n", cv, value);
        cv_set(cv, value);

        ret = nvs_entry_next(&it);
        if (ret != ESP_OK && ret != ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGE(TAG, "Error (%s) obtaining next entry!", esp_err_to_name(ret));
            break;
        }
    }
    nvs_release_iterator(it);
}
