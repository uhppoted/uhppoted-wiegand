cmake_minimum_required(VERSION 3.13)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include($ENV{PICO_SDK_PATH}/tools/CMakeLists.txt)

project(wiegand_usb C CXX ASM)

set(CMAKE_C_STANDARD   11)
set(CMAKE_CXX_STANDARD 17)

set(CORE "../../../core")
set(COMMON "../../common")
set(FATFS "../../../lib/no-OS-FatFS-SD-SPI-RPi-Pico")

add_compile_definitions(SYSDATE=$(SYSDATE))
add_compile_definitions(SYSTIME=$(SYSTIME))
add_compile_definitions(FACILITY_CODE=$(FACILITY_CODE))

pico_sdk_init()

add_executable(wiegand_usb
  ./src/reference.c
  ${COMMON}/src/cli.c
  ${CORE}/src/common.c
  ${CORE}/src/uart.c
  ${CORE}/src/sys.c
  ${CORE}/src/read.c
  ${CORE}/src/write.c
  ${CORE}/src/led.c
  ${CORE}/src/buzzer.c
  ${CORE}/src/relays.c
  ${CORE}/src/acl.c
  ${CORE}/src/logd.c
  ${CORE}/src/sdcard.c
  ${CORE}/src/TPIC6B595.c
  ${CORE}/src/hwconfig.c
  ${CORE}/src/usb.c
  ${CORE}/src/pico/sysled.c
)

target_include_directories(wiegand_usb PRIVATE ${CORE}/include)

add_subdirectory(${FATFS}/FatFs_SPI build)

pico_generate_pio_header(
    wiegand_usb
    ${CMAKE_CURRENT_LIST_DIR}/${CORE}//src/READ.pio
)

pico_generate_pio_header(
    wiegand_usb
    ${CMAKE_CURRENT_LIST_DIR}/${CORE}/src/WRITE.pio
)

pico_enable_stdio_usb (wiegand_usb 1)
pico_enable_stdio_uart(wiegand_usb 0)
pico_add_extra_outputs(wiegand_usb)

target_link_libraries(
    wiegand_usb
    pico_stdlib
    pico_multicore
    hardware_pio
    hardware_rtc
    hardware_watchdog
    FatFs_SPI
)
