cmake_minimum_required(VERSION 3.13)

# initialize pico-sdk from GIT
# (note this can come from environment, CMake cache etc)
set(PICO_SDK_FETCH_FROM_GIT on)

set(FAMILY rp2040)
set(BOARD pico_sdk)

set(TOP ${PICO_TINYUSB_PATH})
set(CFG_TUSB_DEBUG 0)

# pico_sdk_import.cmake is a single file copied from this SDK
# note: this must happen before project()
include(pico_sdk_import.cmake)

#include($ENV{TINYUSB_PATH}/hw/bsp/family_support.cmake)

project(llic64)

pico_sdk_init()

add_executable(${PROJECT_NAME})

# initialize the Raspberry Pi Pico SDK

# rest of your project


target_sources(${PROJECT_NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/board_output.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/hid_app.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
        )


target_include_directories(${PROJECT_NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        )

target_link_libraries(${PROJECT_NAME} pico_stdlib pico_multicore tinyusb_host)

pico_add_extra_outputs(${PROJECT_NAME})