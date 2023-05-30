cmake_minimum_required(VERSION 3.13)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include($ENV{PICO_SDK_PATH}/tools/CMakeLists.txt)

project(wiegand-emulator-usb-wifi C CXX ASM)

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

add_executable(wiegand-emulator-usb-wifi
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
  ../../base/src/logd.c
  ../../base/src/tcpd.c
)

target_include_directories(wiegand-emulator-usb-wifi PRIVATE ../../base/include ./include)

pico_generate_pio_header(
    wiegand-emulator-usb-wifi
    ${CMAKE_CURRENT_LIST_DIR}/../../base/src/READ.pio
)

pico_generate_pio_header(
    wiegand-emulator-usb-wifi
    ${CMAKE_CURRENT_LIST_DIR}/../../base/src/WRITE.pio
)

pico_enable_stdio_usb (wiegand-emulator-usb-wifi 1)
pico_enable_stdio_uart(wiegand-emulator-usb-wifi 0)
pico_add_extra_outputs(wiegand-emulator-usb-wifi)

target_link_libraries(
    wiegand-emulator-usb-wifi
    pico_stdlib
    pico_multicore
    pico_cyw43_arch_lwip_threadsafe_background
    hardware_pio
    hardware_rtc
    hardware_spi
    hardware_watchdog
)
