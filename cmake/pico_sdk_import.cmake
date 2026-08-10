# Locate an installed Pico SDK. Keep this tiny so the upstream SDK remains the
# source of truth. Set PICO_SDK_PATH or initialize external/pico-sdk yourself.
if(DEFINED ENV{PICO_SDK_PATH})
  set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})
endif()
if(NOT PICO_SDK_PATH)
  message(FATAL_ERROR "Set PICO_SDK_PATH to the Raspberry Pi Pico SDK directory")
endif()
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

