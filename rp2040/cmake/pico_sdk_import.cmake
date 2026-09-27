# Minimal, relocatable Pico SDK importer. The SDK can be supplied through
# PICO_SDK_PATH or installed locally by bootstrap.sh under deps/pico-sdk.
if(NOT PICO_SDK_PATH)
  if(DEFINED ENV{PICO_SDK_PATH})
    set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})
  else()
    message(FATAL_ERROR "PICO_SDK_PATH is not set. Run ./bootstrap.sh first.")
  endif()
endif()

get_filename_component(PICO_SDK_PATH "${PICO_SDK_PATH}" REALPATH BASE_DIR "${CMAKE_BINARY_DIR}")
if(NOT EXISTS "${PICO_SDK_PATH}/pico_sdk_init.cmake")
  message(FATAL_ERROR "Pico SDK not found at ${PICO_SDK_PATH}")
endif()
include("${PICO_SDK_PATH}/pico_sdk_init.cmake")
