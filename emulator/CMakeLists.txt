#cmake -DCMAKE_BUILD_TYPE=Release ..
#cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake_minimum_required(VERSION 3.15)
project(SoundDecoderLib)

add_compile_options(-Wall -Wextra -Werror -Wno-unused-variable -Wno-unused-parameter -ftrapv)
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-fsanitize=address,undefined)
    add_link_options(-fsanitize=address,undefined)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-fsanitize=address,undefined,leak)
    add_link_options(-fsanitize=address,undefined,leak)
endif()

add_compile_options(-DCONFIG_MOUNT_POINT=".")

# add_subdirectory(../decodervm)
add_library(SoundDecoderVMLib STATIC ../decodervm/slot.c ../decodervm/vm.c
    ../decodervm/engine.c ../decodervm/utils.c ../decodervm/project.c
    ../decodervm/cv.c ../decodervm/logger.c
    ../audio/wave.c ../audio/mixer.c)
target_include_directories(SoundDecoderVMLib PUBLIC ../include)

add_executable(emulator emulator.c player.c)

target_link_libraries(emulator pulse pulse-simple pthread)

target_include_directories(emulator PUBLIC ../include)
target_link_libraries(emulator SoundDecoderVMLib)
