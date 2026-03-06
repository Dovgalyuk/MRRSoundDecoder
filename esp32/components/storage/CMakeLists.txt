if("${CONFIG_STORAGE_SD}" STREQUAL y)
    set(srcs "sd.c")
endif()
if("${CONFIG_STORAGE_FLASH}" STREQUAL y)
    set(srcs "littlefs.c")
endif()

idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES fatfs
                       WHOLE_ARCHIVE)
