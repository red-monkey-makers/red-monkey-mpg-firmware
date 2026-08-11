#ifndef OPENMPG_BTSTACK_CONFIG_H
#define OPENMPG_BTSTACK_CONFIG_H

// The Pico SDK always compiles BTstack's stdout dump backend, which requires
// the formatting helper even when no dump backend is initialized. Production
// disables USB/UART stdio and never installs that backend; keep informational
// logging itself out of production builds.
#define ENABLE_LOG_ERROR
#define ENABLE_PRINTF_HEXDUMP
#ifndef OPENMPG_PRODUCTION_BUILD
#define ENABLE_LOG_INFO
#endif

// Mobile-client bench support is target-local so BLE-only configuration cannot
// alter the production Bluetooth Classic receiver's BTstack data structures.
#ifdef OPENMPG_ENABLE_MOBILE_BLE
#define ENABLE_LE_PERIPHERAL
#define ENABLE_LE_SECURE_CONNECTIONS
#define ENABLE_MICRO_ECC_FOR_LE_SECURE_CONNECTIONS
#define ENABLE_SOFTWARE_AES128
#define MAX_ATT_DB_SIZE 512
#endif

// Fixed-size BTstack pools: no allocator is used from Bluetooth callbacks.
#define HCI_OUTGOING_PRE_BUFFER_SIZE 4
#define HCI_ACL_PAYLOAD_SIZE (1691 + 4)
#define HCI_ACL_CHUNK_SIZE_ALIGNMENT 4
#define MAX_NR_BTSTACK_LINK_KEY_DB_MEMORY_ENTRIES 2
#define MAX_NR_HCI_CONNECTIONS 1
#define MAX_NR_HID_HOST_CONNECTIONS 1
#define MAX_NR_L2CAP_CHANNELS 4
#define MAX_NR_L2CAP_SERVICES 3
#define MAX_NR_RFCOMM_CHANNELS 1
#define MAX_NR_RFCOMM_MULTIPLEXERS 1
#define MAX_NR_RFCOMM_SERVICES 1
#define MAX_NR_SERVICE_RECORD_ITEMS 1

// Avoid overrunning the shared CYW43 bus.
#define MAX_NR_CONTROLLER_ACL_BUFFERS 3
#define MAX_NR_CONTROLLER_SCO_PACKETS 3
#define ENABLE_HCI_CONTROLLER_TO_HOST_FLOW_CONTROL
#define HCI_HOST_ACL_PACKET_LEN 1024
#define HCI_HOST_ACL_PACKET_NUM 3
#define HCI_HOST_SCO_PACKET_LEN 120
#define HCI_HOST_SCO_PACKET_NUM 3

#define NVM_NUM_DEVICE_DB_ENTRIES 4
#define NVM_NUM_LINK_KEYS 4
#define HAVE_EMBEDDED_TIME_MS
#define HAVE_ASSERT
#define HCI_RESET_RESEND_TIMEOUT_MS 1000

#endif
