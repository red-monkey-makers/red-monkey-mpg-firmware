#pragma once

// Minimal host-test double for the BTstack TLV API used by PersistentConfig.

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  int (*get_tag)(void* context, uint32_t tag, uint8_t* buffer,
                 uint32_t buffer_size);
  int (*store_tag)(void* context, uint32_t tag, const uint8_t* data,
                   uint32_t data_size);
  void (*delete_tag)(void* context, uint32_t tag);
} btstack_tlv_t;

void btstack_tlv_get_instance(const btstack_tlv_t** implementation,
                              void** context);

#ifdef __cplusplus
}
#endif
