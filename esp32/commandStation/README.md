
# Build

Hack for fixing WiFi:

```
static lv_display_t *bsp_display_lcd_init()
{
const bsp_display_config_t disp_config = {
    .max_transfer_sz = 4096, // BSP_LCD_H_RES * BSP_LCD_V_RES * BSP_LCD_BITS_PER_PIXEL / 8,
};
```
