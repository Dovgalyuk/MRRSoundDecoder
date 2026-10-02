#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_memory_utils.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "lvgl.h"
#include "bsp/esp-bsp.h"
#include "bsp/display.h"

#include "logger.h"

#define MAX_TRAINS 5

static lv_obj_t *train_tabs[MAX_TRAINS];
static lv_obj_t *train_names[MAX_TRAINS];

void logger_init(lv_obj_t *tab);
void wifi_init(void);

void app_main(void)
{
    lv_display_t *disp = bsp_display_start();
    bsp_display_backlight_on();

    bsp_display_lock(0);

    lv_obj_t * screen = lv_screen_active();
    // lv_obj_set_style_bg_color(screen, lv_color_hex(0x003a57), 0);
    // lv_obj_set_style_text_color(screen, lv_color_hex(0xffffff), 0);

    lv_obj_t *tabview = lv_tabview_create(screen);
    lv_obj_set_size(tabview, lv_pct(100), lv_pct(100));
    lv_obj_t *tab_main = lv_tabview_add_tab(tabview, "Main");

    for (int i = 0 ; i < MAX_TRAINS ; ++i) {
        lv_obj_t *tab = lv_tabview_add_tab(tabview, "----");
        train_tabs[i] = tab;
        /* Train controls */
        /* Train name */
        lv_obj_t *label = lv_label_create(tab);
        lv_label_set_text(label, "Train name");
        lv_obj_set_align(label, LV_ALIGN_TOP_LEFT);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);
        train_names[i] = label;
        /* Train speed */
        lv_obj_t *slider = lv_slider_create(tab);
        lv_obj_set_size(slider, lv_pct(90), 40);
        //lv_slider_bind_value(slider, subject_value);
        lv_obj_align(slider, LV_ALIGN_BOTTOM_MID, 0, -60);
        /* Brake and Rev buttons */
        lv_obj_t *buttonmatrix = lv_buttonmatrix_create(tab);
        lv_obj_set_align(buttonmatrix, LV_ALIGN_BOTTOM_MID);
        lv_obj_set_size(buttonmatrix, lv_pct(90), 40);
        lv_obj_set_style_border_width(buttonmatrix, 0, 0);
        lv_obj_set_style_pad_all(buttonmatrix, 0, 0);
        static const char * buttonmatrix_map_0[] = {"Fwd", "Brake", NULL};
        lv_buttonmatrix_set_map(buttonmatrix, buttonmatrix_map_0);
        /* Stop button */
        lv_obj_t *stopbutton = lv_button_create(tab);
        lv_obj_set_align(stopbutton, LV_ALIGN_TOP_RIGHT);
        lv_obj_t *label_1 = lv_label_create(stopbutton);
        lv_obj_set_align(label_1, LV_ALIGN_CENTER);
        lv_label_set_text(label_1, "Stop");
    }

    bsp_display_unlock();

    logger_init(tab_main);

    /* Print chip information */
    esp_chip_info_t chip_info;
    uint32_t flash_size;
    esp_chip_info(&chip_info);
    logger_printf("This is %s chip with %d CPU core(s), %s%s%s%s\n",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "",
           (chip_info.features & CHIP_FEATURE_BT) ? "BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "BLE" : "",
           (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");

    if (esp_flash_get_size(NULL, &flash_size) == ESP_OK) {
        logger_printf("%" PRIu32 "MB %s flash\n", flash_size / (uint32_t)(1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
    }
    logger_printf("Free heap size: %" PRIu32 " bytes\n", esp_get_free_heap_size());

    int32_t display_width = lv_disp_get_hor_res(disp);
    int32_t display_height = lv_disp_get_ver_res(disp);
    logger_printf("Display %d x %d\n", display_width, display_height);

    wifi_init();
}