cmake_minimum_required(VERSION 3.13)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include($ENV{PICO_SDK_PATH}/tools/CMakeLists.txt)

project(wiegand-emulator-usb C CXX ASM)

set(CMAKE_C_STANDARD   11)
set(CMAKE_CXX_STANDARD 17)

add_compile_definitions(SYSDATE=$(SYSDATE))
add_compile_definitions(SYSTIME=$(SYSTIME))
add_compile_definitions(FACILITY_CODE=$(FACILITY_CODE))

pico_sdk_init()

add_executable(wiegand-emulator-usb
  src/emulator.c
  ../src/cli.c
  ../../base/src/hwconfig.c
  ../../base/src/common.c
  ../../base/src/sys.c
  ../../base/src/uart.c
  ../../base/src/usb.c
  ../../base/src/read.c
  ../../base/src/write.c
  ../../base/src/led.c
  ../../base/src/buzzer.c
  ../../base/src/relays.c
  ../../base/src/TPIC6B595.c
)

target_include_directories(wiegand-emulator-usb PRIVATE ../../base/include)

pico_generate_pio_header(
    wiegand-emulator-usb
    ${CMAKE_CURRENT_LIST_DIR}/../../base/src/READ.pio
)

pico_generate_pio_header(
    wiegand-emulator-usb
    ${CMAKE_CURRENT_LIST_DIR}/../../base/src/WRITE.pio
)

pico_enable_stdio_usb (wiegand-emulator-usb 1)
pico_enable_stdio_uart(wiegand-emulator-usb 0)
pico_add_extra_outputs(wiegand-emulator-usb)

target_link_libraries(
    wiegand-emulator-usb
    pico_stdlib
    pico_multicore
    hardware_pio
    hardware_rtc
    hardware_spi
    hardware_watchdog
)
