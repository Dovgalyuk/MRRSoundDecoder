set(srcs "player.c" "../../../audio/wave.c")

idf_component_register(SRCS ${srcs}
                       PRIV_REQUIRES decodervm esp_driver_i2s esp_driver_gpio
                       esp_driver_dac esp_driver_gptimer
                       INCLUDE_DIRS "../../../include")
