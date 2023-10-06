cmake_minimum_required(VERSION 3.13)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include($ENV{PICO_SDK_PATH}/tools/CMakeLists.txt)

project(wiegand-controller-wifi-usb C CXX ASM)

set(CMAKE_C_STANDARD   11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

set(CORE "../../../core")
set(COMMON "../../common")
set(FATFS "../../../lib/no-OS-FatFS-SD-SPI-RPi-Pico")

option(PICO_CYW43_ARCH_POLL "Select polling behavior for CYW43 Wi-Fi chip" ON)

add_compile_definitions(SYSDATE=$(SYSDATE))
add_compile_definitions(SYSTIME=$(SYSTIME))
add_compile_definitions(FACILITY_CODE=$(FACILITY_CODE))
add_compile_definitions(MASTER_PASSCODE=${MASTER_PASSCODE})
add_compile_definitions(KEYPAD="${KEYPAD}")

add_compile_definitions(SSID=\"${SSID}\")
add_compile_definitions(PASSWORD=\"${PASSWORD}\")
add_compile_definitions(TCPD_CLI_PORT=${TCPD_CLI_PORT})
add_compile_definitions(TCPD_LOG_PORT=${TCPD_LOG_PORT})
add_compile_definitions(TCPD_SERVER_IDLE=${TCPD_SERVER_IDLE})
add_compile_definitions(TCPD_CLIENT_IDLE=${TCPD_CLIENT_IDLE})
add_compile_definitions(TCPD_POLL_INTERVAL=${TCPD_POLL_INTERVAL})

pico_sdk_init()

add_executable(wiegand-controller-wifi-usb
  ./src/controller.c
  ${COMMON}/src/controller.c
  ${CORE}/src/acl.c
  ${CORE}/src/buzzer.c
  ${COMMON}/src/cli.c
  ${CORE}/src/common.c
  ${CORE}/src/led.c
  ${CORE}/src/logd.c
  ${CORE}/src/read.c
  ${CORE}/src/relays.c
  ${CORE}/src/flash.c
  ${CORE}/src/sdcard.c
  ${CORE}/src/TPIC6B595.c
  ${CORE}/src/uart.c
  ${CORE}/src/usb.c
  ${CORE}/src/sys.c
  ${CORE}/src/cli.c
  ${CORE}/src/hwconfig.c
  ${CORE}/src/picow/cyw43.c
  ${CORE}/src/picow/sysled.c
  ${CORE}/src/picow/tcpd.c
)

target_include_directories(wiegand-controller-wifi-usb PRIVATE ${CORE}/include ./include)

add_subdirectory(${FATFS}/FatFs_SPI build)

pico_generate_pio_header(
    wiegand-controller-wifi-usb
    ${CMAKE_CURRENT_LIST_DIR}/${CORE}/src/READ.pio
)

pico_enable_stdio_usb (wiegand-controller-wifi-usb 1)
pico_enable_stdio_uart(wiegand-controller-wifi-usb 0)
pico_add_extra_outputs(wiegand-controller-wifi-usb)

target_link_libraries(
    wiegand-controller-wifi-usb
    pico_stdlib
    pico_multicore
    pico_cyw43_arch_lwip_threadsafe_background
    hardware_pio
    hardware_rtc
    hardware_spi
    hardware_watchdog
    hardware_flash
    FatFs_SPI
)
