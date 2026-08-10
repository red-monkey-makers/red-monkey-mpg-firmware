cmake_minimum_required(VERSION 3.20)

# Raspberry Pi Pico VS Code extension integration. The extension installs and
# maintains this helper under the user's .pico-sdk directory.
if(WIN32)
  set(USERHOME $ENV{USERPROFILE})
else()
  set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.3.0)
set(toolchainVersion 15_2_Rel1)
set(picotoolVersion 2.3.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if(EXISTS ${picoVscode})
  include(${picoVscode})
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(PICO_BOARD pico2_w CACHE STRING "Pico board type")

option(OPENMPG_BUILD_HOST_TESTS "Build portable host tests" ON)
option(OPENMPG_ENABLE_HOST_SANITIZERS
       "Instrument host tests with AddressSanitizer and UBSan" OFF)
set(OPENMPG_PICO_DEFAULT OFF)
if(DEFINED PICO_SDK_PATH)
  set(OPENMPG_PICO_DEFAULT ON)
endif()
option(OPENMPG_BUILD_PICO "Build Raspberry Pi Pico firmware"
       ${OPENMPG_PICO_DEFAULT})
set(OPENMPG_USB_VID "0xCAFE" CACHE STRING
    "Authorized USB vendor ID (0xCAFE is development-only)")
set(OPENMPG_LITE2_DESCRIPTOR_SHA256 "" CACHE STRING
    "Approved 8BitDo Lite 2 D-mode HID descriptor SHA-256")
set(OPENMPG_RELEASE_VERSION "0.3.2-production-rc1" CACHE STRING
    "Version embedded in the production receiver")
set(OPENMPG_RELEASE_DATE "2026-08-10" CACHE STRING
    "ISO release date embedded in the production receiver")
set(OPENMPG_USB_BCD_DEVICE "0x0302" CACHE STRING
    "USB bcdDevice revision for the production receiver")
if(NOT OPENMPG_RELEASE_VERSION MATCHES
   "^[0-9]+\\.[0-9]+\\.[0-9]+([-+][0-9A-Za-z.-]+)?$")
  message(FATAL_ERROR "OPENMPG_RELEASE_VERSION must be SemVer-like.")
endif()
if(NOT OPENMPG_RELEASE_DATE MATCHES
   "^[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]$")
  message(FATAL_ERROR "OPENMPG_RELEASE_DATE must use YYYY-MM-DD.")
endif()
if(NOT OPENMPG_USB_BCD_DEVICE MATCHES
   "^0[xX][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f]$")
  message(FATAL_ERROR "OPENMPG_USB_BCD_DEVICE must be a four-digit hex value.")
endif()
if(NOT OPENMPG_USB_VID MATCHES "^(0[xX][0-9A-Fa-f]+|[0-9]+)$")
  message(FATAL_ERROR "OPENMPG_USB_VID must be a hexadecimal or decimal 16-bit value.")
endif()
option(OPENMPG_COMMERCIAL_RELEASE
       "Enable commercial release guards (requires an authorized USB VID)" OFF)
option(OPENMPG_SECURE_BOOT_PROVISIONED
       "A reviewed RP2350 signed-boot manufacturing process is active" OFF)
if(NOT OPENMPG_LITE2_DESCRIPTOR_SHA256 STREQUAL "")
  string(LENGTH "${OPENMPG_LITE2_DESCRIPTOR_SHA256}" OPENMPG_DESCRIPTOR_HASH_LENGTH)
  if(NOT OPENMPG_DESCRIPTOR_HASH_LENGTH EQUAL 64 OR
     NOT OPENMPG_LITE2_DESCRIPTOR_SHA256 MATCHES "^[0-9A-Fa-f]+$")
    message(FATAL_ERROR
      "OPENMPG_LITE2_DESCRIPTOR_SHA256 must be exactly 64 hexadecimal characters.")
  endif()
endif()
math(EXPR OPENMPG_USB_VID_VALUE "${OPENMPG_USB_VID}")
if(OPENMPG_USB_VID_VALUE LESS 1 OR OPENMPG_USB_VID_VALUE GREATER 65535)
  message(FATAL_ERROR "OPENMPG_USB_VID must be a 16-bit non-zero value.")
endif()
if(OPENMPG_COMMERCIAL_RELEASE AND OPENMPG_USB_VID_VALUE EQUAL 51966)
  message(FATAL_ERROR
    "Commercial releases cannot use the development USB VID 0xCAFE. "
    "Set OPENMPG_USB_VID to an authorized value.")
endif()
if(OPENMPG_COMMERCIAL_RELEASE AND NOT OPENMPG_SECURE_BOOT_PROVISIONED)
  message(FATAL_ERROR
    "Commercial releases require a reviewed signed-boot provisioning process. "
    "Do not enable OPENMPG_SECURE_BOOT_PROVISIONED merely to bypass this guard.")
endif()
if(OPENMPG_COMMERCIAL_RELEASE AND OPENMPG_LITE2_DESCRIPTOR_SHA256 STREQUAL "")
  message(FATAL_ERROR
    "Commercial releases require an approved Lite 2 HID descriptor SHA-256.")
endif()

if(OPENMPG_BUILD_PICO)
  include(cmake/pico_sdk_import.cmake)
endif()

project(openmpg LANGUAGES C CXX ASM)

if(OPENMPG_BUILD_HOST_TESTS)
  enable_testing()
  add_library(openmpg_core
    firmware/src/controller_profile.cpp
    firmware/src/control_mapper.cpp
    firmware/src/lite2_report.cpp
    firmware/src/masso_keyboard.cpp
    firmware/src/production_config.cpp
  )
  target_include_directories(openmpg_core PUBLIC firmware/include)
  target_compile_features(openmpg_core PUBLIC cxx_std_17)
  target_compile_options(openmpg_core PRIVATE -Wall -Wextra -Wpedantic -Werror)

  add_executable(openmpg_core_tests firmware/tests/control_mapper_test.cpp)
  target_link_libraries(openmpg_core_tests PRIVATE openmpg_core)
  target_compile_options(openmpg_core_tests PRIVATE -UNDEBUG -Wall -Wextra -Wpedantic -Werror)
  add_test(NAME openmpg_core_tests COMMAND openmpg_core_tests)

  add_executable(openmpg_controller_profile_tests
    firmware/tests/controller_profile_test.cpp)
  target_link_libraries(openmpg_controller_profile_tests PRIVATE openmpg_core)
  target_compile_options(openmpg_controller_profile_tests PRIVATE
    -UNDEBUG -Wall -Wextra -Wpedantic -Werror)
  add_test(NAME openmpg_controller_profile_tests
           COMMAND openmpg_controller_profile_tests)

  add_executable(openmpg_safety_properties_tests
    firmware/tests/safety_properties_test.cpp)
  target_link_libraries(openmpg_safety_properties_tests PRIVATE openmpg_core)
  target_compile_options(openmpg_safety_properties_tests PRIVATE
    -UNDEBUG -Wall -Wextra -Wpedantic -Werror)
  add_test(NAME openmpg_safety_properties_tests
           COMMAND openmpg_safety_properties_tests)

  add_executable(openmpg_config_service_tests
    firmware/tests/config_service_test.cpp
    firmware/src/config_service.cpp)
  target_include_directories(openmpg_config_service_tests PRIVATE firmware/usb)
  target_link_libraries(openmpg_config_service_tests PRIVATE openmpg_core)
  target_compile_options(openmpg_config_service_tests PRIVATE
    -UNDEBUG -Wall -Wextra -Wpedantic -Werror)
  add_test(NAME openmpg_config_service_tests COMMAND openmpg_config_service_tests)

  add_executable(openmpg_persistent_config_tests
    firmware/tests/persistent_config_test.cpp
    firmware/src/persistent_config.cpp)
  target_include_directories(openmpg_persistent_config_tests BEFORE PRIVATE
    firmware/tests/fakes)
  target_link_libraries(openmpg_persistent_config_tests PRIVATE openmpg_core)
  target_compile_options(openmpg_persistent_config_tests PRIVATE
    -UNDEBUG -Wall -Wextra -Wpedantic -Werror)
  add_test(NAME openmpg_persistent_config_tests
           COMMAND openmpg_persistent_config_tests)

  if(OPENMPG_ENABLE_HOST_SANITIZERS AND
     CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    foreach(target openmpg_core openmpg_core_tests openmpg_controller_profile_tests
                   openmpg_safety_properties_tests
                   openmpg_config_service_tests openmpg_persistent_config_tests)
      target_compile_options(${target} PRIVATE
        -fsanitize=address,undefined -fno-omit-frame-pointer)
      target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    endforeach()
  endif()
endif()

if(OPENMPG_BUILD_PICO)
  pico_sdk_init()

  add_executable(openmpg EXCLUDE_FROM_ALL
    firmware/src/main.cpp
    firmware/src/controller_profile.cpp
    firmware/src/control_mapper.cpp
    firmware/src/lite2_report.cpp
    firmware/src/masso_keyboard.cpp
    firmware/src/platform_stub.cpp
  )
  target_include_directories(openmpg PRIVATE firmware/include)
  target_compile_features(openmpg PRIVATE cxx_std_17)
  # USB HID stays deliberately unlinked until the descriptor and fail-safe
  # sender land together; the current platform adapter is inert.
  target_link_libraries(openmpg PRIVATE pico_stdlib pico_cyw43_arch_none)
  pico_enable_stdio_usb(openmpg 0)
  pico_enable_stdio_uart(openmpg 1)
  pico_add_extra_outputs(openmpg)

  # Safe bench diagnostic: scans Bluetooth Classic devices and writes discovery
  # data to USB serial. It contains no USB HID keyboard descriptor or output.
  add_executable(openmpg_bt_scan EXCLUDE_FROM_ALL firmware/diagnostics/bt_scan.c)
  target_include_directories(openmpg_bt_scan PRIVATE firmware/config)
  target_link_libraries(openmpg_bt_scan PRIVATE
    pico_stdlib
    pico_cyw43_arch_none
    pico_btstack_cyw43
    pico_btstack_classic
  )
  pico_enable_stdio_usb(openmpg_bt_scan 1)
  pico_enable_stdio_uart(openmpg_bt_scan 0)
  pico_add_extra_outputs(openmpg_bt_scan)

  set(OPENMPG_CONTROLLER_ADDRESS "E4:17:D8:32:66:02" CACHE STRING
      "Commissioning controller Bluetooth address")
  add_executable(openmpg_hid_dump EXCLUDE_FROM_ALL
    firmware/diagnostics/hid_dump.c
    firmware/src/controller_profile.cpp
    firmware/src/descriptor_identity.c
    firmware/src/control_mapper.cpp
    firmware/src/lite2_report.cpp
    firmware/src/input_diagnostic.cpp
  )
  target_include_directories(openmpg_hid_dump PRIVATE
    firmware/config
    firmware/include
  )
  target_compile_features(openmpg_hid_dump PRIVATE cxx_std_17)
  target_compile_definitions(openmpg_hid_dump PRIVATE
    OPENMPG_CONTROLLER_ADDRESS="${OPENMPG_CONTROLLER_ADDRESS}")
  target_link_libraries(openmpg_hid_dump PRIVATE
    pico_stdlib
    pico_sha256
    pico_cyw43_arch_none
    pico_btstack_cyw43
    pico_btstack_classic
  )
  pico_enable_stdio_usb(openmpg_hid_dump 1)
  pico_enable_stdio_uart(openmpg_hid_dump 0)
  pico_add_extra_outputs(openmpg_hid_dump)

  # Mac-only USB keyboard commissioning image. It has no Bluetooth input and
  # emits one bounded test sequence after an eight-second delay.
  add_executable(openmpg_keyboard_bench EXCLUDE_FROM_ALL
    firmware/diagnostics/keyboard_bench.cpp
    firmware/src/masso_keyboard.cpp
    firmware/usb/usb_descriptors.c
  )
  target_include_directories(openmpg_keyboard_bench PRIVATE
    firmware/include
    firmware/usb
  )
  target_compile_features(openmpg_keyboard_bench PRIVATE cxx_std_17)
  target_link_libraries(openmpg_keyboard_bench PRIVATE
    pico_stdlib
    tinyusb_device
  )
  pico_enable_stdio_usb(openmpg_keyboard_bench 0)
  pico_enable_stdio_uart(openmpg_keyboard_bench 0)
  pico_add_extra_outputs(openmpg_keyboard_bench)

  # Full controller-to-keyboard path for validation on a normal computer.
  # This remains a bench image until live behavior is commissioned.
  add_executable(openmpg_live_bridge_bench EXCLUDE_FROM_ALL
    firmware/diagnostics/live_bridge_bench.cpp
    firmware/src/controller_profile.cpp
    firmware/src/control_mapper.cpp
    firmware/src/lite2_report.cpp
    firmware/src/masso_keyboard.cpp
    firmware/usb/usb_keyboard_device.c
    firmware/usb/usb_descriptors.c
  )
  target_include_directories(openmpg_live_bridge_bench PRIVATE
    firmware/config
    firmware/include
    firmware/usb
  )
  target_compile_features(openmpg_live_bridge_bench PRIVATE cxx_std_17)
  target_compile_definitions(openmpg_live_bridge_bench PRIVATE
    OPENMPG_CONTROLLER_ADDRESS="${OPENMPG_CONTROLLER_ADDRESS}"
    OPENMPG_USB_PRODUCT="Red Monkey MPG Live Bridge Bench"
    OPENMPG_USB_SERIAL="BRIDGE-BENCH-0001"
    OPENMPG_USB_PID=0x4014
  )
  target_link_libraries(openmpg_live_bridge_bench PRIVATE
    pico_stdlib
    pico_cyw43_arch_none
    pico_btstack_cyw43
    pico_btstack_classic
    tinyusb_device
  )
  pico_enable_stdio_usb(openmpg_live_bridge_bench 0)
  pico_enable_stdio_uart(openmpg_live_bridge_bench 0)
  pico_add_extra_outputs(openmpg_live_bridge_bench)

  # Version-pinned machine commissioning image. Build only after the complete
  # Mac live-bridge checklist passes for this exact controller.
  add_executable(openmpg_masso_g3_513_commissioning EXCLUDE_FROM_ALL
    firmware/diagnostics/live_bridge_bench.cpp
    firmware/src/controller_profile.cpp
    firmware/src/control_mapper.cpp
    firmware/src/lite2_report.cpp
    firmware/src/masso_keyboard.cpp
    firmware/usb/usb_keyboard_device.c
    firmware/usb/usb_descriptors.c
  )
  target_include_directories(openmpg_masso_g3_513_commissioning PRIVATE
    firmware/config firmware/include firmware/usb)
  target_compile_features(openmpg_masso_g3_513_commissioning PRIVATE cxx_std_17)
  target_compile_definitions(openmpg_masso_g3_513_commissioning PRIVATE
    OPENMPG_CONTROLLER_ADDRESS="${OPENMPG_CONTROLLER_ADDRESS}"
    OPENMPG_USB_PRODUCT="Red Monkey MPG MASSO G3 5.13 Commissioning"
    OPENMPG_USB_SERIAL="G3-513-COMMISSION-0001"
    OPENMPG_USB_PID=0x4018
  )
  target_link_libraries(openmpg_masso_g3_513_commissioning PRIVATE
    pico_stdlib pico_cyw43_arch_none pico_btstack_cyw43
    pico_btstack_classic tinyusb_device)
  pico_enable_stdio_usb(openmpg_masso_g3_513_commissioning 0)
  pico_enable_stdio_uart(openmpg_masso_g3_513_commissioning 0)
  pico_add_extra_outputs(openmpg_masso_g3_513_commissioning)

  # Mac-only production configuration preview. This composite HID + CDC image
  # is never connected to MASSO until pairing, persistence, and fault tests pass.
  add_executable(openmpg_configurable_receiver_preview EXCLUDE_FROM_ALL
    firmware/diagnostics/live_bridge_bench.cpp
    firmware/src/config_service.cpp
    firmware/src/controller_profile.cpp
    firmware/src/control_mapper.cpp
    firmware/src/descriptor_identity.c
    firmware/src/lite2_report.cpp
    firmware/src/masso_keyboard.cpp
    firmware/src/persistent_config.cpp
    firmware/src/production_config.cpp
    firmware/usb/usb_keyboard_device.c
    firmware/usb/usb_descriptors.c
  )
  target_include_directories(openmpg_configurable_receiver_preview PRIVATE
    firmware/config firmware/include firmware/usb)
  target_compile_features(openmpg_configurable_receiver_preview PRIVATE cxx_std_17)
  target_compile_definitions(openmpg_configurable_receiver_preview PRIVATE
    OPENMPG_CONTROLLER_ADDRESS="${OPENMPG_CONTROLLER_ADDRESS}"
    OPENMPG_ENABLE_CONFIG_CDC=1
    OPENMPG_LITE2_DESCRIPTOR_SHA256="${OPENMPG_LITE2_DESCRIPTOR_SHA256}"
    OPENMPG_POLL_BTSTACK=1
    CYW43_LWIP=0
    OPENMPG_FIRMWARE_VERSION="0.3.2-config-preview"
    OPENMPG_USB_PRODUCT="Red Monkey MPG Configurable Receiver Preview"
    OPENMPG_USB_SERIAL="CONFIG-PREVIEW-0001"
    OPENMPG_USB_PID=0x401C
  )
  target_link_libraries(openmpg_configurable_receiver_preview PRIVATE
    pico_stdlib pico_unique_id pico_sha256 pico_cyw43_arch_poll pico_btstack_cyw43
    pico_btstack_classic pico_btstack_flash_bank tinyusb_device)
  pico_enable_stdio_usb(openmpg_configurable_receiver_preview 0)
  pico_enable_stdio_uart(openmpg_configurable_receiver_preview 0)
  pico_add_extra_outputs(openmpg_configurable_receiver_preview)

  # Production receiver: one composite keyboard + fail-closed setup image. It ships
  # unpaired, persists the app-selected controller/mapping, and enables keyboard
  # output only when the CDC setup session is closed.
  add_executable(openmpg_production_receiver
    firmware/diagnostics/live_bridge_bench.cpp
    firmware/src/config_service.cpp
    firmware/src/controller_profile.cpp
    firmware/src/control_mapper.cpp
    firmware/src/descriptor_identity.c
    firmware/src/lite2_report.cpp
    firmware/src/masso_keyboard.cpp
    firmware/src/persistent_config.cpp
    firmware/src/production_config.cpp
    firmware/usb/usb_keyboard_device.c
    firmware/usb/usb_descriptors.c
  )
  target_include_directories(openmpg_production_receiver PRIVATE
    firmware/config firmware/include firmware/usb)
  target_compile_features(openmpg_production_receiver PRIVATE cxx_std_17)
  target_compile_definitions(openmpg_production_receiver PRIVATE
    OPENMPG_CONTROLLER_ADDRESS=""
    OPENMPG_PRODUCTION_BUILD=1
    OPENMPG_ENABLE_CONFIG_CDC=1
    OPENMPG_LITE2_DESCRIPTOR_SHA256="${OPENMPG_LITE2_DESCRIPTOR_SHA256}"
    OPENMPG_POLL_BTSTACK=1
    CYW43_LWIP=0
    OPENMPG_USB_DYNAMIC_SERIAL=1
    OPENMPG_FIRMWARE_VERSION="${OPENMPG_RELEASE_VERSION}"
    PICO_PROGRAM_BUILD_DATE="${OPENMPG_RELEASE_DATE}"
    OPENMPG_USB_PRODUCT="Red Monkey MPG Receiver"
    OPENMPG_USB_SERIAL="OPENMPG"
    OPENMPG_USB_VID=${OPENMPG_USB_VID}
    OPENMPG_USB_PID=0x4020
    OPENMPG_USB_BCD_DEVICE=${OPENMPG_USB_BCD_DEVICE}
  )
  # Pico SDK sources are compiled into this target and carry their own warning
  # policy. Apply strict warnings only to Red Monkey MPG-owned production sources.
  set_property(SOURCE
    firmware/diagnostics/live_bridge_bench.cpp
    firmware/src/config_service.cpp
    firmware/src/controller_profile.cpp
    firmware/src/control_mapper.cpp
    firmware/src/descriptor_identity.c
    firmware/src/lite2_report.cpp
    firmware/src/masso_keyboard.cpp
    firmware/src/persistent_config.cpp
    firmware/src/production_config.cpp
    firmware/usb/usb_keyboard_device.c
    firmware/usb/usb_descriptors.c
    APPEND PROPERTY COMPILE_OPTIONS
    -Wall -Wextra -Wformat=2 -Wshadow -Werror=return-type)
  target_link_libraries(openmpg_production_receiver PRIVATE
    pico_stdlib pico_unique_id pico_sha256 pico_cyw43_arch_poll pico_btstack_cyw43
    pico_btstack_classic pico_btstack_flash_bank tinyusb_device)
  pico_enable_stdio_usb(openmpg_production_receiver 0)
  pico_enable_stdio_uart(openmpg_production_receiver 0)
  pico_set_program_version(openmpg_production_receiver
                           "${OPENMPG_RELEASE_VERSION}")
  pico_set_program_description(openmpg_production_receiver
                               "Red Monkey MPG CNC receiver release candidate")
  pico_add_extra_outputs(openmpg_production_receiver)
endif()
