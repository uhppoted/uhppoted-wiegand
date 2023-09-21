cmake_minimum_required(VERSION 3.13)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include($ENV{PICO_SDK_PATH}/tools/CMakeLists.txt)

project(wiegand-emulator-picow C CXX ASM)

set(CMAKE_C_STANDARD   11)
set(CMAKE_CXX_STANDARD 17)

set(CORE "../../../core")
set(COMMON "../../common")

add_compile_definitions(SYSDATE=$(SYSDATE))
add_compile_definitions(SYSTIME=$(SYSTIME))
add_compile_definitions(FACILITY_CODE=$(FACILITY_CODE))

pico_sdk_init()

add_executable(wiegand-emulator-picow
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
  ${CORE}/src/picow/cyw43.c
  ${CORE}/src/picow/sysled.c
)

target_include_directories(wiegand-emulator-picow PRIVATE ${CORE}/include ./include)

pico_generate_pio_header(
    wiegand-emulator-picow
    ${CMAKE_CURRENT_LIST_DIR}/${CORE}/src/READ.pio
)

pico_generate_pio_header(
    wiegand-emulator-picow
    ${CMAKE_CURRENT_LIST_DIR}/${CORE}/src/WRITE.pio
)

pico_enable_stdio_usb (wiegand-emulator-picow 0)
pico_enable_stdio_uart(wiegand-emulator-picow 1)
pico_add_extra_outputs(wiegand-emulator-picow)

target_link_libraries(
    wiegand-emulator-picow
    pico_stdlib
    pico_multicore
    pico_cyw43_arch_none
    hardware_pio
    hardware_rtc
    hardware_spi
    hardware_watchdog
)
