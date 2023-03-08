cmake_minimum_required(VERSION 3.13)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include($ENV{PICO_SDK_PATH}/tools/CMakeLists.txt)

project(wiegand C CXX ASM)

set(CMAKE_C_STANDARD   11)
set(CMAKE_CXX_STANDARD 17)

add_compile_definitions(SYSDATE=$(SYSDATE))
add_compile_definitions(SYSTIME=$(SYSTIME))
add_compile_definitions(FACILITY_CODE=$(FACILITY_CODE))

pico_sdk_init()

add_executable(wiegand
  base/wiegand.c
  base/sys.c
  base/cli.c
  base/read.c
  base/write.c
  base/led.c
  base/buzzer.c
  base/relays.c
  base/acl.c
  base/sdcard.c
  base/TPIC6B595.c
  base/hwconfig.c
)

add_subdirectory(lib/no-OS-FatFS-SD-SPI-RPi-Pico/FatFs_SPI build)

pico_generate_pio_header(
    wiegand
    ${CMAKE_CURRENT_LIST_DIR}/base/READ.pio
)

pico_generate_pio_header(
    wiegand
    ${CMAKE_CURRENT_LIST_DIR}/base/WRITE.pio
)

pico_generate_pio_header(
    wiegand
    ${CMAKE_CURRENT_LIST_DIR}/base/LED.pio
)

pico_generate_pio_header(
    wiegand
    ${CMAKE_CURRENT_LIST_DIR}/base/BLINK.pio
)

pico_generate_pio_header(
    wiegand
    ${CMAKE_CURRENT_LIST_DIR}/base/BUZZER.pio
)

pico_enable_stdio_usb (wiegand 0)
pico_enable_stdio_uart(wiegand 1)
pico_add_extra_outputs(wiegand)

target_link_libraries(
    wiegand 
    pico_stdlib
    pico_multicore
    hardware_pio
    hardware_rtc
    hardware_watchdog
    FatFs_SPI
)
