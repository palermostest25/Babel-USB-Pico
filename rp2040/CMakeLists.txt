cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico CACHE STRING "RP2040 board")

if(NOT PICO_SDK_PATH AND EXISTS "${CMAKE_CURRENT_LIST_DIR}/deps/pico-sdk/pico_sdk_init.cmake")
  set(PICO_SDK_PATH "${CMAKE_CURRENT_LIST_DIR}/deps/pico-sdk")
endif()

if(NOT PICO_TINYUSB_PATH AND EXISTS "${CMAKE_CURRENT_LIST_DIR}/deps/tinyusb/src/class/mtp/mtp_device.c")
  set(PICO_TINYUSB_PATH "${CMAKE_CURRENT_LIST_DIR}/deps/tinyusb")
endif()

include(cmake/pico_sdk_import.cmake)

project(babel_usb_rp2040 C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

if(NOT EXISTS "${PICO_TINYUSB_PATH}/src/class/mtp/mtp_device.c")
  message(FATAL_ERROR
    "This project needs TinyUSB 0.21.0 (or newer) with the MTP device class. "
    "Run ./bootstrap.sh, or set PICO_TINYUSB_PATH to a compatible checkout.")
endif()

add_executable(babel_usb
  src/main.c
  src/babel.c
  src/mtp_babel.c
  src/usb_descriptors.c
)

target_include_directories(babel_usb PRIVATE src)
target_compile_options(babel_usb PRIVATE -Wall -Wextra)
target_link_libraries(babel_usb PRIVATE
  pico_stdlib
  pico_unique_id
  tinyusb_device
  tinyusb_board
)

# TinyUSB owns USB; enabling Pico SDK USB stdio would create a conflicting CDC interface.
pico_enable_stdio_usb(babel_usb 0)
pico_enable_stdio_uart(babel_usb 0)
pico_add_extra_outputs(babel_usb)
