cmake_minimum_required(VERSION 3.13)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(pico_dht22 C CXX ASM)
pico_sdk_init()

add_executable(dht22_reader
    main.c
)

target_link_libraries(dht22_reader
    pico_stdlib
    hardware_gpio
    hardware_adc
    hardware_i2c
)

pico_enable_stdio_usb(dht22_reader 1)
pico_enable_stdio_uart(dht22_reader 0)
pico_add_extra_outputs(dht22_reader)