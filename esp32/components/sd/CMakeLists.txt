set(srcs "sd.c")

idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES fatfs
                       WHOLE_ARCHIVE)
