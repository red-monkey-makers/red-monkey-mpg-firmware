#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool openmpg_descriptor_sha256(const uint8_t* descriptor, size_t length,
                               uint8_t output[32]);
bool openmpg_descriptor_sha256_hex(const uint8_t* descriptor, size_t length,
                                   char output[65]);
bool openmpg_descriptor_matches_sha256(const uint8_t* descriptor,
                                       size_t length,
                                       const char* expected_hex);

#ifdef __cplusplus
}
#endif
