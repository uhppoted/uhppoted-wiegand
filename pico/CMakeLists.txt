cmake_minimum_required(VERSION 3.13)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include($ENV{PICO_SDK_PATH}/tools/CMakeLists.txt)

project(wiegand C CXX ASM)

set(CMAKE_C_STANDARD   11)
set(CMAKE_CXX_STANDARD 17)

add_compile_definitions(SYSDATE=$(SYSDATE))
add_compile_definitions(SYSTIME=$(SYSTIME))
add_compile_definitions(FACILITY_CODE=$(FACILITY_CODE))

pico_sdk_init()

add_executable(wiegand
  src/wiegand.c
  src/sys.c
  src/cli.c
  src/reader.c
  src/writer.c
  src/led.c
  src/buzzer.c
  src/acl.c
)

pico_generate_pio_header(
    wiegand
    ${CMAKE_CURRENT_LIST_DIR}/src/READ.pio
)

pico_generate_pio_header(
    wiegand
    ${CMAKE_CURRENT_LIST_DIR}/src/WRITE.pio
)

pico_generate_pio_header(
    wiegand
    ${CMAKE_CURRENT_LIST_DIR}/src/LED.pio
)

pico_generate_pio_header(
    wiegand
    ${CMAKE_CURRENT_LIST_DIR}/src/BLINK.pio
)

pico_generate_pio_header(
    wiegand
    ${CMAKE_CURRENT_LIST_DIR}/src/BUZZER.pio
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
)
