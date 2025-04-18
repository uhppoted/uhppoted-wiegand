cmake_minimum_required(VERSION 3.13)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include($ENV{PICO_SDK_PATH}/tools/CMakeLists.txt)

project(wiegand C CXX ASM)

set(CMAKE_C_STANDARD   11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

set(CORE "../../../core")
set(COMMON "../../common")
set(FATFS "../../../lib/no-OS-FatFS-SD-SPI-RPi-Pico")

add_compile_definitions(SYSDATE=$(SYSDATE))
add_compile_definitions(SYSTIME=$(SYSTIME))
add_compile_definitions(FACILITY_CODE=$(FACILITY_CODE))
add_compile_definitions(MASTER_PASSCODE=${MASTER_PASSCODE})
add_compile_definitions(KEYPAD="${KEYPAD}")

pico_sdk_init()

add_executable(wiegand
  ./src/universal.c
  ${COMMON}/src/cli.c
  ${COMMON}/src/universal.c
  ${CORE}/src/common.c
  ${CORE}/src/uart.c
  ${CORE}/src/sys.c
  ${CORE}/src/read.c
  ${CORE}/src/write.c
  ${CORE}/src/keypad.c
  ${CORE}/src/led.c
  ${CORE}/src/buzzer.c
  ${CORE}/src/relays.c
  ${CORE}/src/acl.c
  ${CORE}/src/logd.c
  ${CORE}/src/flash.c
  ${CORE}/src/sdcard.c
  ${CORE}/src/TPIC6B595.c
  ${CORE}/src/cli.c
  ${CORE}/src/hwconfig.c
  ${CORE}/src/pico/sysled.c
)

target_include_directories(wiegand PRIVATE ${CORE}/include)

add_subdirectory(${FATFS}/FatFs_SPI build)

pico_generate_pio_header(
    wiegand
    ${CMAKE_CURRENT_LIST_DIR}/${CORE}/src/READ.pio
)

pico_generate_pio_header(
    wiegand
    ${CMAKE_CURRENT_LIST_DIR}/${CORE}/src/WRITE.pio
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
    hardware_flash
    FatFs_SPI
)
