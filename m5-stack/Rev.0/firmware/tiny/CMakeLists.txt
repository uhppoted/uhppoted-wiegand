cmake_minimum_required(VERSION 3.5)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include($ENV{PICO_SDK_PATH}/tools/CMakeLists.txt)

project(wiegand C CXX ASM)

set(CMAKE_C_STANDARD   11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

add_compile_definitions(PICO_ENTER_USB_BOOT_ON_EXIT=1)
add_compile_definitions(PARAM_ASSERTIONS_ENABLE_ALL=1)

add_compile_definitions(VERSION="${VERSION}")
add_compile_definitions(KEYPAD="${KEYPAD}")
add_compile_definitions(FACILITY_CODE=${FACILITY_CODE})

pico_sdk_init()

set(TARGET_NAME "wiegand")
set(CORE "../core")

add_executable(${TARGET_NAME}
  src/main.c
  src/sys.c
  src/hwconfig.c

  ${CORE}/src/M5.c
  ${CORE}/src/sys.c
  ${CORE}/src/log.c
  ${CORE}/src/cli.c
  ${CORE}/src/ANSI.c
  ${CORE}/src/usb.c
  ${CORE}/src/buffer.c
  ${CORE}/src/SK6812.c
  ${CORE}/src/LED.c
  ${CORE}/src/wiegand.c
)

pico_generate_pio_header(
    ${TARGET_NAME}
    ${CMAKE_CURRENT_LIST_DIR}/src/ws2812.pio
    ${CMAKE_CURRENT_LIST_DIR}/../core/src/sk6812.pio
    ${CMAKE_CURRENT_LIST_DIR}/../core/src/wiegand.pio
)

pico_set_program_name(${TARGET_NAME} "uhppoted-wiegand-emulator")
pico_set_program_version(${TARGET_NAME} "0.0")

pico_enable_stdio_usb (${TARGET_NAME} 1)
pico_enable_stdio_uart(${TARGET_NAME} 0)
pico_add_extra_outputs(${TARGET_NAME})

# Ref. https://tinygo.org/docs/reference/microcontrollers/machine/waveshare-rp2040-zero/
target_compile_definitions(${TARGET_NAME} PRIVATE
  PICO_XOSC_STARTUP_DELAY_MULTIPLIER=64
)

target_include_directories(${TARGET_NAME} 
   PRIVATE include
   PRIVATE ${CORE}/include
)

target_link_libraries(
    ${TARGET_NAME} 

    pico_stdlib

    hardware_pio
    hardware_watchdog
)
