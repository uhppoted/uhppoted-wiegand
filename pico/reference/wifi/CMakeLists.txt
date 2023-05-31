cmake_minimum_required(VERSION 3.13)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include($ENV{PICO_SDK_PATH}/tools/CMakeLists.txt)

project(wiegand_wifi C CXX ASM)

set(CMAKE_C_STANDARD   11)
set(CMAKE_CXX_STANDARD 17)

option(PICO_CYW43_ARCH_POLL "Select polling behavior for CYW43 Wi-Fi chip" ON)

add_compile_definitions(SYSDATE=$(SYSDATE))
add_compile_definitions(SYSTIME=$(SYSTIME))
add_compile_definitions(FACILITY_CODE=$(FACILITY_CODE))

add_compile_definitions(SSID=\"${SSID}\")
add_compile_definitions(PASSWORD=\"${PASSWORD}\")
add_compile_definitions(TCPD_CLI_PORT=${TCPD_CLI_PORT})
add_compile_definitions(TCPD_LOG_PORT=${TCPD_LOG_PORT})
add_compile_definitions(TCPD_SERVER_IDLE=${TCPD_SERVER_IDLE})
add_compile_definitions(TCPD_CLIENT_IDLE=${TCPD_CLIENT_IDLE})
add_compile_definitions(TCPD_POLL_INTERVAL=${TCPD_POLL_INTERVAL})

pico_sdk_init()

add_executable(wiegand_wifi
  ./src/reference.c
  ../src/cli.c
  ../../base/src/hwconfig.c
  ../../base/src/common.c
  ../../base/src/sys.c
  ../../base/src/uart.c
  ../../base/src/read.c
  ../../base/src/write.c
  ../../base/src/led.c
  ../../base/src/buzzer.c
  ../../base/src/relays.c
  ../../base/src/acl.c
  ../../base/src/logd.c
  ../../base/src/sdcard.c
  ../../base/src/TPIC6B595.c
  ../../base/src/tcpd.c
)

target_include_directories(wiegand_wifi PRIVATE ../../base/include ./include)

add_subdirectory(../../lib/no-OS-FatFS-SD-SPI-RPi-Pico/FatFs_SPI build)

pico_generate_pio_header(
    wiegand_wifi
    ${CMAKE_CURRENT_LIST_DIR}/../../base/src/READ.pio
)

pico_generate_pio_header(
    wiegand_wifi
    ${CMAKE_CURRENT_LIST_DIR}/../../base/src/WRITE.pio
)

pico_enable_stdio_usb (wiegand_wifi 0)
pico_enable_stdio_uart(wiegand_wifi 1)
pico_add_extra_outputs(wiegand_wifi)

target_link_libraries(
    wiegand_wifi
    pico_stdlib
    pico_multicore
    pico_cyw43_arch_lwip_threadsafe_background
    hardware_pio
    hardware_rtc
    hardware_watchdog
    FatFs_SPI
)
