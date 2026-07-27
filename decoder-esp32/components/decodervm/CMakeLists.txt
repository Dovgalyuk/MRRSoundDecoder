set(srcs "../../../decodervm/vm.c" "../../../decodervm/slot.c"
    "../../../decodervm/engine.c" "../../../decodervm/utils.c"
    "../../../decodervm/project.c" "../../../decodervm/cv.c"
    "../../../decodervm/logger.c")

idf_component_register(SRCS ${srcs}
                       PRIV_REQUIRES player
                       INCLUDE_DIRS "../../../include")
