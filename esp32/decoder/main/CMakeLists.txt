idf_component_register(SRCS "main.c"
                       PRIV_REQUIRES spi_flash decodervm player storage web engine esp_timer esp_psram wifi
                       INCLUDE_DIRS ".")
