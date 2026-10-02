set(srcs motor.c output.c)

if("${CONFIG_BOARD_VERSION}" GREATER_EQUAL 3)
    list(APPEND srcs drv8235.c)
endif()

idf_component_register(SRCS ${srcs}
                       PRIV_REQUIRES decodervm esp_driver_ledc esp_driver_gpio esp_driver_i2c
                       INCLUDE_DIRS "../../../../include" "../../include")
