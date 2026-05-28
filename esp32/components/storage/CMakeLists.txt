idf_component_register(SRCS littlefs.c cv.c
                       PRIV_REQUIRES esp_driver_spi esp_driver_gpio spi_flash
                                     esp_partition vfs nvs_flash
                       INCLUDE_DIRS "." "../../include" "../../../include"
                       WHOLE_ARCHIVE)
