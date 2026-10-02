#ifndef PINS_H
#define PINS_H

#include "sdkconfig.h"

#if CONFIG_IDF_TARGET_ESP32S3
/* Sound */
#   if CONFIG_BOARD_VERSION==1
#       define I2S_BCLK_IO1         12
#       define I2S_WS_IO1           13
#       define I2S_DOUT_IO1         11
#   elif CONFIG_BOARD_VERSION>=2
#       define I2S_BCLK_IO1         46
#       define I2S_WS_IO1           9
#       define I2S_DOUT_IO1         3
#   endif
/* Motor */
#   if CONFIG_BOARD_VERSION==2
#       define MOTOR_INPUT_V        1
#       define MOTOR_ADC_UNIT       ADC_UNIT_1
#       define MOTOR_ADC_CHANNEL    ADC_CHANNEL_0
#   elif CONFIG_BOARD_VERSION==3
#       define MOTOR_INPUT_FAULT      1
#       define MOTOR_SCL              38
#       define MOTOR_SDA              39
#   endif
#   define MOTOR_OUTPUT_DIR1          2
#   define MOTOR_OUTPUT_DIR2          4
/* Physical outputs */
#   if CONFIG_BOARD_VERSION==1
#       define PHYS_OUTPUT_SMOKE      47
#       define PHYS_OUTPUT_FWD_LIGHT  48
#       define PHYS_OUTPUT_BACK_LIGHT 42
#       define PHYS_OUTPUT_4          41
#       define PHYS_OUTPUT_5          40
#       define PHYS_OUTPUT_6          39
#       define PHYS_OUTPUT_7          38
#   elif CONFIG_BOARD_VERSION>=2
#       define PHYS_OUTPUT_SMOKE      48
#       define PHYS_OUTPUT_FWD_LIGHT  47
#       define PHYS_OUTPUT_BACK_LIGHT 21
#       define PHYS_OUTPUT_4          14
#       define PHYS_OUTPUT_5          13
#       define PHYS_OUTPUT_6          12
#       define PHYS_OUTPUT_7          11
#   endif
/* Storage */
#   if CONFIG_BOARD_VERSION==1
#       define FLASH_SPI_D0         8
#       define FLASH_SPI_D1         15
#       define FLASH_SPI_D2         16
#       define FLASH_SPI_D3         17
#       define FLASH_SPI_CLK        18
#       define FLASH_SPI_CS         7
#   endif
#else
#   error Define pins for peripherals
#endif

#endif
