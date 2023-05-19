cmake_minimum_required(VERSION 3.13)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include($ENV{PICO_SDK_PATH}/tools/CMakeLists.txt)

project(wiegand-controller-w C CXX ASM)

set(CMAKE_C_STANDARD   11)
set(CMAKE_CXX_STANDARD 17)

option(PICO_CYW43_ARCH_POLL "Select polling behavior for CYW43 Wi-Fi chip" ON)

add_compile_definitions(SSID=\"${SSID}\")
add_compile_definitions(PASSWORD=\"${PASSWORD}\")
add_compile_definitions(SYSDATE=$(SYSDATE))
add_compile_definitions(SYSTIME=$(SYSTIME))
add_compile_definitions(FACILITY_CODE=$(FACILITY_CODE))

pico_sdk_init()

add_executable(wiegand-controller-w
  src/controller.c
  ../src/cli.c
  ../src/logd.c
  ../../base/src/hwconfig.c
  ../../base/src/common.c
  ../../base/src/uart.c
  ../../base/src/sys.c
  ../../base/src/read.c
  ../../base/src/led.c
  ../../base/src/buzzer.c
  ../../base/src/relays.c
  ../../base/src/TPIC6B595.c
  ../../base/src/sdcard.c
  ../../base/src/acl.c
  ../../base/src/tcpd.c
)

target_include_directories(wiegand-controller-w PRIVATE ../../base/include ./include)

add_subdirectory(../../lib/no-OS-FatFS-SD-SPI-RPi-Pico/FatFs_SPI build)

pico_generate_pio_header(
    wiegand-controller-w
    ${CMAKE_CURRENT_LIST_DIR}/../../base/src/READ.pio
)

pico_enable_stdio_usb (wiegand-controller-w 0)
pico_enable_stdio_uart(wiegand-controller-w 1)
pico_add_extra_outputs(wiegand-controller-w)

target_link_libraries(
    wiegand-controller-w
    pico_stdlib
    pico_multicore
    pico_cyw43_arch_lwip_threadsafe_background
    hardware_pio
    hardware_rtc
    hardware_spi
    hardware_watchdog
    FatFs_SPI
)
