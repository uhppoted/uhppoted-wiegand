cmake_minimum_required(VERSION 3.13)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include($ENV{PICO_SDK_PATH}/tools/CMakeLists.txt)

project(wiegand-emulator-wifi-usb C CXX ASM)

set(CMAKE_C_STANDARD   11)
set(CMAKE_CXX_STANDARD 17)

set(CORE "../../../core")
set(COMMON "../../common")

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

add_executable(wiegand-emulator-wifi-usb
  ./src/emulator.c
  ${COMMON}/src/cli.c
  ${CORE}/src/common.c
  ${CORE}/src/uart.c
  ${CORE}/src/sys.c
  ${CORE}/src/read.c
  ${CORE}/src/write.c
  ${CORE}/src/led.c
  ${CORE}/src/buzzer.c
  ${CORE}/src/relays.c
  ${CORE}/src/logd.c
  ${CORE}/src/TPIC6B595.c
  ${CORE}/src/cli.c
  ${CORE}/src/hwconfig.c
  ${CORE}/src/usb.c
  ${CORE}/src/picow/cyw43.c
  ${CORE}/src/picow/sysled.c
  ${CORE}/src/picow/tcpd.c
)

target_include_directories(wiegand-emulator-wifi-usb PRIVATE ${CORE}/include ./include)

pico_generate_pio_header(
    wiegand-emulator-wifi-usb
    ${CMAKE_CURRENT_LIST_DIR}/${CORE}/src/READ.pio
)

pico_generate_pio_header(
    wiegand-emulator-wifi-usb
    ${CMAKE_CURRENT_LIST_DIR}/${CORE}/src/WRITE.pio
)

pico_enable_stdio_usb (wiegand-emulator-wifi-usb 1)
pico_enable_stdio_uart(wiegand-emulator-wifi-usb 0)
pico_add_extra_outputs(wiegand-emulator-wifi-usb)

target_link_libraries(
    wiegand-emulator-wifi-usb
    pico_stdlib
    pico_multicore
    pico_cyw43_arch_lwip_threadsafe_background
    hardware_pio
    hardware_rtc
    hardware_spi
    hardware_watchdog
)
