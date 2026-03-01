set(srcs "main.c")

idf_component_register(SRCS ${srcs}
                       PRIV_REQUIRES decodervm esp_driver_gptimer
                       INCLUDE_DIRS "../../../include")
