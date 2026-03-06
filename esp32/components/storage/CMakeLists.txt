if("${CONFIG_STORAGE_SD}" STREQUAL y)
    set(srcs "sd.c")
endif()
if("${CONFIG_STORAGE_FLASH}" STREQUAL y)
    set(srcs "spiffs.c")
endif()

idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES fatfs spiffs
                       WHOLE_ARCHIVE)
