set(srcs "main.c" "../../../decodervm/vm.c" "../../../decodervm/slot.c"
    "../../../decodervm/engine.c")

idf_component_register(SRCS ${srcs}
                       PRIV_REQUIRES player
                       INCLUDE_DIRS "../../../include")
