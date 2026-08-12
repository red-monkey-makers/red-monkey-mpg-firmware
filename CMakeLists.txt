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

option(RED_MONKEY_MPG_BUILD_HOST_TESTS "Build portable host tests" ON)
option(RED_MONKEY_MPG_ENABLE_HOST_SANITIZERS
       "Instrument host tests with AddressSanitizer and UBSan" OFF)
set(RED_MONKEY_MPG_PICO_DEFAULT OFF)
if(DEFINED PICO_SDK_PATH)
  set(RED_MONKEY_MPG_PICO_DEFAULT ON)
endif()
option(RED_MONKEY_MPG_BUILD_PICO "Build Raspberry Pi Pico firmware"
       ${RED_MONKEY_MPG_PICO_DEFAULT})
set(RED_MONKEY_MPG_USB_VID "0xCAFE" CACHE STRING
    "Authorized USB vendor ID (0xCAFE is development-only)")
set(RED_MONKEY_MPG_LITE2_DESCRIPTOR_SHA256 "" CACHE STRING
    "Approved 8BitDo Lite 2 D-mode HID descriptor SHA-256")
set(RED_MONKEY_MPG_RELEASE_VERSION "0.4.0-rc.3" CACHE STRING
    "Version embedded in the production receiver")
set(RED_MONKEY_MPG_RELEASE_DATE "2026-08-12" CACHE STRING
    "ISO release date embedded in the production receiver")
set(RED_MONKEY_MPG_USB_BCD_DEVICE "0x0402" CACHE STRING
    "USB bcdDevice revision for the production receiver")
if(NOT RED_MONKEY_MPG_RELEASE_VERSION MATCHES
   "^[0-9]+\\.[0-9]+\\.[0-9]+([-+][0-9A-Za-z.-]+)?$")
  message(FATAL_ERROR "RED_MONKEY_MPG_RELEASE_VERSION must be SemVer-like.")
endif()
if(NOT RED_MONKEY_MPG_RELEASE_DATE MATCHES
   "^[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]$")
  message(FATAL_ERROR "RED_MONKEY_MPG_RELEASE_DATE must use YYYY-MM-DD.")
endif()
if(NOT RED_MONKEY_MPG_USB_BCD_DEVICE MATCHES
   "^0[xX][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f]$")
  message(FATAL_ERROR "RED_MONKEY_MPG_USB_BCD_DEVICE must be a four-digit hex value.")
endif()
if(NOT RED_MONKEY_MPG_USB_VID MATCHES "^(0[xX][0-9A-Fa-f]+|[0-9]+)$")
  message(FATAL_ERROR "RED_MONKEY_MPG_USB_VID must be a hexadecimal or decimal 16-bit value.")
endif()
option(RED_MONKEY_MPG_COMMERCIAL_RELEASE
       "Enable commercial release guards (requires an authorized USB VID)" OFF)
option(RED_MONKEY_MPG_SECURE_BOOT_PROVISIONED
       "A reviewed RP2350 signed-boot manufacturing process is active" OFF)
if(NOT RED_MONKEY_MPG_LITE2_DESCRIPTOR_SHA256 STREQUAL "")
  string(LENGTH "${RED_MONKEY_MPG_LITE2_DESCRIPTOR_SHA256}" RED_MONKEY_MPG_DESCRIPTOR_HASH_LENGTH)
  if(NOT RED_MONKEY_MPG_DESCRIPTOR_HASH_LENGTH EQUAL 64 OR
     NOT RED_MONKEY_MPG_LITE2_DESCRIPTOR_SHA256 MATCHES "^[0-9A-Fa-f]+$")
    message(FATAL_ERROR
      "RED_MONKEY_MPG_LITE2_DESCRIPTOR_SHA256 must be exactly 64 hexadecimal characters.")
  endif()
endif()
math(EXPR RED_MONKEY_MPG_USB_VID_VALUE "${RED_MONKEY_MPG_USB_VID}")
if(RED_MONKEY_MPG_USB_VID_VALUE LESS 1 OR RED_MONKEY_MPG_USB_VID_VALUE GREATER 65535)
  message(FATAL_ERROR "RED_MONKEY_MPG_USB_VID must be a 16-bit non-zero value.")
endif()
if(RED_MONKEY_MPG_COMMERCIAL_RELEASE AND RED_MONKEY_MPG_USB_VID_VALUE EQUAL 51966)
  message(FATAL_ERROR
    "Commercial releases cannot use the development USB VID 0xCAFE. "
    "Set RED_MONKEY_MPG_USB_VID to an authorized value.")
endif()
if(RED_MONKEY_MPG_COMMERCIAL_RELEASE AND NOT RED_MONKEY_MPG_SECURE_BOOT_PROVISIONED)
  message(FATAL_ERROR
    "Commercial releases require a reviewed signed-boot provisioning process. "
    "Do not enable RED_MONKEY_MPG_SECURE_BOOT_PROVISIONED merely to bypass this guard.")
endif()
if(RED_MONKEY_MPG_COMMERCIAL_RELEASE AND RED_MONKEY_MPG_LITE2_DESCRIPTOR_SHA256 STREQUAL "")
  message(FATAL_ERROR
    "Commercial releases require an approved Lite 2 HID descriptor SHA-256.")
endif()

if(RED_MONKEY_MPG_BUILD_PICO)
  include(cmake/pico_sdk_import.cmake)
endif()

project(red_monkey_mpg LANGUAGES C CXX ASM)

if(RED_MONKEY_MPG_BUILD_HOST_TESTS)
  enable_testing()
  add_library(red_monkey_mpg_core
    firmware/src/controller_profile.cpp
    firmware/src/control_mapper.cpp
    firmware/src/lite2_report.cpp
    firmware/src/masso_keyboard.cpp
    firmware/src/mobile_protocol.cpp
    firmware/src/production_config.cpp
  )
  target_include_directories(red_monkey_mpg_core PUBLIC firmware/include)
  target_compile_features(red_monkey_mpg_core PUBLIC cxx_std_17)
  target_compile_options(red_monkey_mpg_core PRIVATE -Wall -Wextra -Wpedantic -Werror)

  add_executable(red_monkey_mpg_core_tests firmware/tests/control_mapper_test.cpp)
  target_link_libraries(red_monkey_mpg_core_tests PRIVATE red_monkey_mpg_core)
  target_compile_options(red_monkey_mpg_core_tests PRIVATE -UNDEBUG -Wall -Wextra -Wpedantic -Werror)
  add_test(NAME red_monkey_mpg_core_tests COMMAND red_monkey_mpg_core_tests)

  add_executable(red_monkey_mpg_controller_profile_tests
    firmware/tests/controller_profile_test.cpp)
  target_link_libraries(red_monkey_mpg_controller_profile_tests PRIVATE red_monkey_mpg_core)
  target_compile_options(red_monkey_mpg_controller_profile_tests PRIVATE
    -UNDEBUG -Wall -Wextra -Wpedantic -Werror)
  add_test(NAME red_monkey_mpg_controller_profile_tests
           COMMAND red_monkey_mpg_controller_profile_tests)

  add_executable(red_monkey_mpg_safety_properties_tests
    firmware/tests/safety_properties_test.cpp)
  target_link_libraries(red_monkey_mpg_safety_properties_tests PRIVATE red_monkey_mpg_core)
  target_compile_options(red_monkey_mpg_safety_properties_tests PRIVATE
    -UNDEBUG -Wall -Wextra -Wpedantic -Werror)
  add_test(NAME red_monkey_mpg_safety_properties_tests
           COMMAND red_monkey_mpg_safety_properties_tests)

  add_executable(red_monkey_mpg_config_service_tests
    firmware/tests/config_service_test.cpp
    firmware/src/config_service.cpp)
  target_include_directories(red_monkey_mpg_config_service_tests PRIVATE firmware/usb)
  target_link_libraries(red_monkey_mpg_config_service_tests PRIVATE red_monkey_mpg_core)
  target_compile_options(red_monkey_mpg_config_service_tests PRIVATE
    -UNDEBUG -Wall -Wextra -Wpedantic -Werror)
  add_test(NAME red_monkey_mpg_config_service_tests COMMAND red_monkey_mpg_config_service_tests)

  add_executable(red_monkey_mpg_persistent_config_tests
    firmware/tests/persistent_config_test.cpp
    firmware/src/persistent_config.cpp)
  target_include_directories(red_monkey_mpg_persistent_config_tests BEFORE PRIVATE
    firmware/tests/fakes)
  target_link_libraries(red_monkey_mpg_persistent_config_tests PRIVATE red_monkey_mpg_core)
  target_compile_options(red_monkey_mpg_persistent_config_tests PRIVATE
    -UNDEBUG -Wall -Wextra -Wpedantic -Werror)
  add_test(NAME red_monkey_mpg_persistent_config_tests
           COMMAND red_monkey_mpg_persistent_config_tests)

  add_executable(red_monkey_mpg_mobile_protocol_tests
    firmware/tests/mobile_protocol_test.cpp)
  target_link_libraries(red_monkey_mpg_mobile_protocol_tests PRIVATE red_monkey_mpg_core)
  target_compile_options(red_monkey_mpg_mobile_protocol_tests PRIVATE
    -UNDEBUG -Wall -Wextra -Wpedantic -Werror)
  add_test(NAME red_monkey_mpg_mobile_protocol_tests COMMAND red_monkey_mpg_mobile_protocol_tests)

  add_executable(red_monkey_mpg_mobile_bridge_tests
    firmware/tests/mobile_bridge_test.cpp)
  target_link_libraries(red_monkey_mpg_mobile_bridge_tests PRIVATE red_monkey_mpg_core)
  target_compile_options(red_monkey_mpg_mobile_bridge_tests PRIVATE
    -UNDEBUG -Wall -Wextra -Wpedantic -Werror)
  add_test(NAME red_monkey_mpg_mobile_bridge_tests COMMAND red_monkey_mpg_mobile_bridge_tests)

  if(RED_MONKEY_MPG_ENABLE_HOST_SANITIZERS AND
     CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    foreach(target red_monkey_mpg_core red_monkey_mpg_core_tests red_monkey_mpg_controller_profile_tests
                   red_monkey_mpg_safety_properties_tests
                   red_monkey_mpg_config_service_tests red_monkey_mpg_persistent_config_tests
                   red_monkey_mpg_mobile_protocol_tests red_monkey_mpg_mobile_bridge_tests)
      target_compile_options(${target} PRIVATE
        -fsanitize=address,undefined -fno-omit-frame-pointer)
      target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    endforeach()
  endif()
endif()

if(RED_MONKEY_MPG_BUILD_PICO)
  pico_sdk_init()

  add_executable(red_monkey_mpg EXCLUDE_FROM_ALL
    firmware/src/main.cpp
    firmware/src/controller_profile.cpp
    firmware/src/control_mapper.cpp
    firmware/src/lite2_report.cpp
    firmware/src/masso_keyboard.cpp
    firmware/src/platform_stub.cpp
  )
  target_include_directories(red_monkey_mpg PRIVATE firmware/include)
  target_compile_features(red_monkey_mpg PRIVATE cxx_std_17)
  # USB HID stays deliberately unlinked until the descriptor and fail-safe
  # sender land together; the current platform adapter is inert.
  target_link_libraries(red_monkey_mpg PRIVATE pico_stdlib pico_cyw43_arch_none)
  pico_enable_stdio_usb(red_monkey_mpg 0)
  pico_enable_stdio_uart(red_monkey_mpg 1)
  pico_add_extra_outputs(red_monkey_mpg)

  # Safe bench diagnostic: scans Bluetooth Classic devices and writes discovery
  # data to USB serial. It contains no USB HID keyboard descriptor or output.
  add_executable(red_monkey_mpg_bt_scan EXCLUDE_FROM_ALL firmware/diagnostics/bt_scan.c)
  target_include_directories(red_monkey_mpg_bt_scan PRIVATE firmware/config)
  target_link_libraries(red_monkey_mpg_bt_scan PRIVATE
    pico_stdlib
    pico_cyw43_arch_none
    pico_btstack_cyw43
    pico_btstack_classic
  )
  pico_enable_stdio_usb(red_monkey_mpg_bt_scan 1)
  pico_enable_stdio_uart(red_monkey_mpg_bt_scan 0)
  pico_add_extra_outputs(red_monkey_mpg_bt_scan)

  set(RED_MONKEY_MPG_CONTROLLER_ADDRESS "00:00:5E:00:53:01" CACHE STRING
      "Commissioning controller Bluetooth address")
  add_executable(red_monkey_mpg_hid_dump EXCLUDE_FROM_ALL
    firmware/diagnostics/hid_dump.c
    firmware/src/controller_profile.cpp
    firmware/src/descriptor_identity.c
    firmware/src/control_mapper.cpp
    firmware/src/lite2_report.cpp
    firmware/src/input_diagnostic.cpp
  )
  target_include_directories(red_monkey_mpg_hid_dump PRIVATE
    firmware/config
    firmware/include
  )
  target_compile_features(red_monkey_mpg_hid_dump PRIVATE cxx_std_17)
  target_compile_definitions(red_monkey_mpg_hid_dump PRIVATE
    RED_MONKEY_MPG_CONTROLLER_ADDRESS="${RED_MONKEY_MPG_CONTROLLER_ADDRESS}")
  target_link_libraries(red_monkey_mpg_hid_dump PRIVATE
    pico_stdlib
    pico_sha256
    pico_cyw43_arch_none
    pico_btstack_cyw43
    pico_btstack_classic
  )
  pico_enable_stdio_usb(red_monkey_mpg_hid_dump 1)
  pico_enable_stdio_uart(red_monkey_mpg_hid_dump 0)
  pico_add_extra_outputs(red_monkey_mpg_hid_dump)

  # Safe iPhone protocol diagnostic. It advertises the mobile BLE service and
  # validates every command frame, but deliberately links no TinyUSB keyboard
  # descriptor or CNC output implementation.
  add_executable(red_monkey_mpg_mobile_ble_bench EXCLUDE_FROM_ALL
    firmware/diagnostics/mobile_ble_bench.cpp
    firmware/src/mobile_protocol.cpp
  )
  target_include_directories(red_monkey_mpg_mobile_ble_bench PRIVATE
    firmware/config firmware/include)
  target_compile_features(red_monkey_mpg_mobile_ble_bench PRIVATE cxx_std_17)
  target_compile_definitions(red_monkey_mpg_mobile_ble_bench PRIVATE
    CYW43_LWIP=0
    RED_MONKEY_MPG_ENABLE_MOBILE_BLE=1
    RED_MONKEY_MPG_MOBILE_USB_KEYBOARD=0)
  target_link_libraries(red_monkey_mpg_mobile_ble_bench PRIVATE
    pico_stdlib
    pico_cyw43_arch_poll
    pico_btstack_cyw43
    pico_btstack_ble
    pico_btstack_flash_bank
  )
  pico_btstack_make_gatt_header(red_monkey_mpg_mobile_ble_bench PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/firmware/bluetooth/mobile_jogger.gatt")
  pico_enable_stdio_usb(red_monkey_mpg_mobile_ble_bench 1)
  pico_enable_stdio_uart(red_monkey_mpg_mobile_ble_bench 0)
  pico_add_extra_outputs(red_monkey_mpg_mobile_ble_bench)

  # Mobile-to-CNC production bridge. This remains a separately flashed image
  # from the Bluetooth gamepad receiver so either path has a known rollback.
  # The shared BLE implementation keeps the diagnostic parser and production
  # parser identical; only this target links the TinyUSB keyboard output.
  add_executable(red_monkey_mpg_mobile_receiver EXCLUDE_FROM_ALL
    firmware/diagnostics/mobile_ble_bench.cpp
    firmware/src/control_mapper.cpp
    firmware/src/masso_keyboard.cpp
    firmware/src/mobile_protocol.cpp
    firmware/usb/usb_keyboard_device.c
    firmware/usb/usb_descriptors.c
  )
  target_include_directories(red_monkey_mpg_mobile_receiver PRIVATE
    firmware/config firmware/include firmware/usb)
  target_compile_features(red_monkey_mpg_mobile_receiver PRIVATE cxx_std_17)
  target_compile_definitions(red_monkey_mpg_mobile_receiver PRIVATE
    CYW43_LWIP=0
    RED_MONKEY_MPG_ENABLE_MOBILE_BLE=1
    RED_MONKEY_MPG_MOBILE_USB_KEYBOARD=1
    RED_MONKEY_MPG_USB_DYNAMIC_SERIAL=1
    RED_MONKEY_MPG_USB_PRODUCT="Red Monkey MPG Mobile Receiver"
    RED_MONKEY_MPG_USB_SERIAL="RED_MONKEY_MPG_MOBILE"
    RED_MONKEY_MPG_USB_VID=${RED_MONKEY_MPG_USB_VID}
    RED_MONKEY_MPG_USB_PID=0x4024
    RED_MONKEY_MPG_USB_BCD_DEVICE=${RED_MONKEY_MPG_USB_BCD_DEVICE}
  )
  set_property(SOURCE
    firmware/diagnostics/mobile_ble_bench.cpp
    firmware/src/control_mapper.cpp
    firmware/src/masso_keyboard.cpp
    firmware/src/mobile_protocol.cpp
    firmware/usb/usb_keyboard_device.c
    firmware/usb/usb_descriptors.c
    APPEND PROPERTY COMPILE_OPTIONS
    -Wall -Wextra -Wformat=2 -Wshadow -Werror=return-type)
  target_link_libraries(red_monkey_mpg_mobile_receiver PRIVATE
    pico_stdlib pico_unique_id pico_cyw43_arch_poll pico_btstack_cyw43
    pico_btstack_ble pico_btstack_flash_bank tinyusb_device)
  pico_btstack_make_gatt_header(red_monkey_mpg_mobile_receiver PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/firmware/bluetooth/mobile_jogger.gatt")
  pico_enable_stdio_usb(red_monkey_mpg_mobile_receiver 0)
  pico_enable_stdio_uart(red_monkey_mpg_mobile_receiver 0)
  pico_set_program_version(red_monkey_mpg_mobile_receiver
                           "${RED_MONKEY_MPG_RELEASE_VERSION}")
  pico_set_program_description(red_monkey_mpg_mobile_receiver
                               "Red Monkey MPG mobile CNC receiver")
  set_target_properties(red_monkey_mpg_mobile_receiver PROPERTIES
    OUTPUT_NAME "red-monkey-mpg-mobile-receiver")
  pico_add_extra_outputs(red_monkey_mpg_mobile_receiver)

  # Mac-only USB keyboard commissioning image. It has no Bluetooth input and
  # emits one bounded test sequence after an eight-second delay.
  add_executable(red_monkey_mpg_keyboard_bench EXCLUDE_FROM_ALL
    firmware/diagnostics/keyboard_bench.cpp
    firmware/src/masso_keyboard.cpp
    firmware/usb/usb_descriptors.c
  )
  target_include_directories(red_monkey_mpg_keyboard_bench PRIVATE
    firmware/include
    firmware/usb
  )
  target_compile_features(red_monkey_mpg_keyboard_bench PRIVATE cxx_std_17)
  target_link_libraries(red_monkey_mpg_keyboard_bench PRIVATE
    pico_stdlib
    tinyusb_device
  )
  pico_enable_stdio_usb(red_monkey_mpg_keyboard_bench 0)
  pico_enable_stdio_uart(red_monkey_mpg_keyboard_bench 0)
  pico_add_extra_outputs(red_monkey_mpg_keyboard_bench)

  # Full controller-to-keyboard path for validation on a normal computer.
  # This remains a bench image until live behavior is commissioned.
  add_executable(red_monkey_mpg_live_bridge_bench EXCLUDE_FROM_ALL
    firmware/diagnostics/live_bridge_bench.cpp
    firmware/src/controller_profile.cpp
    firmware/src/control_mapper.cpp
    firmware/src/lite2_report.cpp
    firmware/src/masso_keyboard.cpp
    firmware/usb/usb_keyboard_device.c
    firmware/usb/usb_descriptors.c
  )
  target_include_directories(red_monkey_mpg_live_bridge_bench PRIVATE
    firmware/config
    firmware/include
    firmware/usb
  )
  target_compile_features(red_monkey_mpg_live_bridge_bench PRIVATE cxx_std_17)
  target_compile_definitions(red_monkey_mpg_live_bridge_bench PRIVATE
    RED_MONKEY_MPG_CONTROLLER_ADDRESS="${RED_MONKEY_MPG_CONTROLLER_ADDRESS}"
    RED_MONKEY_MPG_USB_PRODUCT="Red Monkey MPG Live Bridge Bench"
    RED_MONKEY_MPG_USB_SERIAL="BRIDGE-BENCH-0001"
    RED_MONKEY_MPG_USB_PID=0x4014
  )
  target_link_libraries(red_monkey_mpg_live_bridge_bench PRIVATE
    pico_stdlib
    pico_cyw43_arch_none
    pico_btstack_cyw43
    pico_btstack_classic
    tinyusb_device
  )
  pico_enable_stdio_usb(red_monkey_mpg_live_bridge_bench 0)
  pico_enable_stdio_uart(red_monkey_mpg_live_bridge_bench 0)
  pico_add_extra_outputs(red_monkey_mpg_live_bridge_bench)

  # Version-pinned machine commissioning image. Build only after the complete
  # Mac live-bridge checklist passes for this exact controller.
  add_executable(red_monkey_mpg_masso_g3_513_commissioning EXCLUDE_FROM_ALL
    firmware/diagnostics/live_bridge_bench.cpp
    firmware/src/controller_profile.cpp
    firmware/src/control_mapper.cpp
    firmware/src/lite2_report.cpp
    firmware/src/masso_keyboard.cpp
    firmware/usb/usb_keyboard_device.c
    firmware/usb/usb_descriptors.c
  )
  target_include_directories(red_monkey_mpg_masso_g3_513_commissioning PRIVATE
    firmware/config firmware/include firmware/usb)
  target_compile_features(red_monkey_mpg_masso_g3_513_commissioning PRIVATE cxx_std_17)
  target_compile_definitions(red_monkey_mpg_masso_g3_513_commissioning PRIVATE
    RED_MONKEY_MPG_CONTROLLER_ADDRESS="${RED_MONKEY_MPG_CONTROLLER_ADDRESS}"
    RED_MONKEY_MPG_USB_PRODUCT="Red Monkey MPG MASSO G3 5.13 Commissioning"
    RED_MONKEY_MPG_USB_SERIAL="G3-513-COMMISSION-0001"
    RED_MONKEY_MPG_USB_PID=0x4018
  )
  target_link_libraries(red_monkey_mpg_masso_g3_513_commissioning PRIVATE
    pico_stdlib pico_cyw43_arch_none pico_btstack_cyw43
    pico_btstack_classic tinyusb_device)
  pico_enable_stdio_usb(red_monkey_mpg_masso_g3_513_commissioning 0)
  pico_enable_stdio_uart(red_monkey_mpg_masso_g3_513_commissioning 0)
  pico_add_extra_outputs(red_monkey_mpg_masso_g3_513_commissioning)

  # Mac-only production configuration preview. This composite HID + CDC image
  # is never connected to MASSO until pairing, persistence, and fault tests pass.
  add_executable(red_monkey_mpg_configurable_receiver_preview EXCLUDE_FROM_ALL
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
  target_include_directories(red_monkey_mpg_configurable_receiver_preview PRIVATE
    firmware/config firmware/include firmware/usb)
  target_compile_features(red_monkey_mpg_configurable_receiver_preview PRIVATE cxx_std_17)
  target_compile_definitions(red_monkey_mpg_configurable_receiver_preview PRIVATE
    RED_MONKEY_MPG_CONTROLLER_ADDRESS="${RED_MONKEY_MPG_CONTROLLER_ADDRESS}"
    RED_MONKEY_MPG_ENABLE_CONFIG_CDC=1
    RED_MONKEY_MPG_LITE2_DESCRIPTOR_SHA256="${RED_MONKEY_MPG_LITE2_DESCRIPTOR_SHA256}"
    RED_MONKEY_MPG_POLL_BTSTACK=1
    CYW43_LWIP=0
    RED_MONKEY_MPG_FIRMWARE_VERSION="0.3.2-config-preview"
    RED_MONKEY_MPG_USB_PRODUCT="Red Monkey MPG Configurable Receiver Preview"
    RED_MONKEY_MPG_USB_SERIAL="CONFIG-PREVIEW-0001"
    RED_MONKEY_MPG_USB_PID=0x401C
  )
  target_link_libraries(red_monkey_mpg_configurable_receiver_preview PRIVATE
    pico_stdlib pico_unique_id pico_sha256 pico_cyw43_arch_poll pico_btstack_cyw43
    pico_btstack_classic pico_btstack_flash_bank tinyusb_device)
  pico_enable_stdio_usb(red_monkey_mpg_configurable_receiver_preview 0)
  pico_enable_stdio_uart(red_monkey_mpg_configurable_receiver_preview 0)
  pico_add_extra_outputs(red_monkey_mpg_configurable_receiver_preview)

  # Production receiver: one composite keyboard + fail-closed setup image. It ships
  # unpaired, persists the app-selected controller/mapping, and enables keyboard
  # output only when the CDC setup session is closed.
  add_executable(red_monkey_mpg_production_receiver
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
  target_include_directories(red_monkey_mpg_production_receiver PRIVATE
    firmware/config firmware/include firmware/usb)
  target_compile_features(red_monkey_mpg_production_receiver PRIVATE cxx_std_17)
  target_compile_definitions(red_monkey_mpg_production_receiver PRIVATE
    RED_MONKEY_MPG_CONTROLLER_ADDRESS=""
    RED_MONKEY_MPG_PRODUCTION_BUILD=1
    RED_MONKEY_MPG_ENABLE_CONFIG_CDC=1
    RED_MONKEY_MPG_LITE2_DESCRIPTOR_SHA256="${RED_MONKEY_MPG_LITE2_DESCRIPTOR_SHA256}"
    RED_MONKEY_MPG_POLL_BTSTACK=1
    CYW43_LWIP=0
    RED_MONKEY_MPG_USB_DYNAMIC_SERIAL=1
    RED_MONKEY_MPG_FIRMWARE_VERSION="${RED_MONKEY_MPG_RELEASE_VERSION}"
    PICO_PROGRAM_BUILD_DATE="${RED_MONKEY_MPG_RELEASE_DATE}"
    RED_MONKEY_MPG_USB_PRODUCT="Red Monkey MPG Receiver"
    RED_MONKEY_MPG_USB_SERIAL="RED_MONKEY_MPG"
    RED_MONKEY_MPG_USB_VID=${RED_MONKEY_MPG_USB_VID}
    RED_MONKEY_MPG_USB_PID=0x4020
    RED_MONKEY_MPG_USB_BCD_DEVICE=${RED_MONKEY_MPG_USB_BCD_DEVICE}
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
  target_link_libraries(red_monkey_mpg_production_receiver PRIVATE
    pico_stdlib pico_unique_id pico_sha256 pico_cyw43_arch_poll pico_btstack_cyw43
    pico_btstack_classic pico_btstack_flash_bank tinyusb_device)
  pico_enable_stdio_usb(red_monkey_mpg_production_receiver 0)
  pico_enable_stdio_uart(red_monkey_mpg_production_receiver 0)
  pico_set_program_version(red_monkey_mpg_production_receiver
                           "${RED_MONKEY_MPG_RELEASE_VERSION}")
  pico_set_program_description(red_monkey_mpg_production_receiver
                               "Red Monkey MPG CNC receiver release candidate")
  set_target_properties(red_monkey_mpg_production_receiver PROPERTIES
    OUTPUT_NAME "red-monkey-mpg-gamepad-receiver")
  pico_add_extra_outputs(red_monkey_mpg_production_receiver)
endif()
