#include "lvgl.h"
#include "bsp/esp-bsp.h"
#include "bsp/display.h"

#define MESSAGE_LEN 256

static lv_obj_t *ta;

void logger_init(lv_obj_t *tab)
{
    bsp_display_lock(0);
    ta = lv_textarea_create(tab);
    lv_obj_set_align(ta, LV_ALIGN_CENTER);
    lv_obj_set_size(ta, lv_pct(100), lv_pct(100));
    bsp_display_unlock();
}

void logger_printf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char *buf = malloc(MESSAGE_LEN + 1);
    size_t len = vsnprintf(buf, MESSAGE_LEN, fmt, args);
    buf[len] = 0;
    va_end(args);

    printf("%s", buf);
    if (ta) {
        bsp_display_lock(0);
        lv_textarea_add_text(ta, buf);
        bsp_display_unlock();
    }
    free(buf);
}
