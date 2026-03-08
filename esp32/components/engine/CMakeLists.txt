set(srcs "engine.c")

idf_component_register(SRCS ${srcs}
                       PRIV_REQUIRES decodervm esp_driver_ledc esp_driver_gpio
                       INCLUDE_DIRS "../../../include")
