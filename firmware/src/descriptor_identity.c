#include "red_monkey_mpg/descriptor_identity.h"

#include <string.h>

#include "pico/sha256.h"

static int hex_nibble(char value) {
  if (value >= '0' && value <= '9') return value - '0';
  if (value >= 'a' && value <= 'f') return value - 'a' + 10;
  if (value >= 'A' && value <= 'F') return value - 'A' + 10;
  return -1;
}

bool red_monkey_mpg_descriptor_sha256(const uint8_t* descriptor, size_t length,
                               uint8_t output[32]) {
  if (descriptor == NULL || output == NULL || length == 0) return false;
  pico_sha256_state_t state = {0};
  if (pico_sha256_start_blocking_until(&state, SHA256_BIG_ENDIAN, false,
                                       make_timeout_time_ms(50)) != PICO_OK) {
    return false;
  }
  pico_sha256_update_blocking(&state, descriptor, length);
  sha256_result_t result;
  pico_sha256_finish(&state, &result);
  memcpy(output, result.bytes, sizeof(result.bytes));
  return true;
}

bool red_monkey_mpg_descriptor_sha256_hex(const uint8_t* descriptor, size_t length,
                                   char output[65]) {
  static const char hex[] = "0123456789abcdef";
  uint8_t digest[32];
  if (output == NULL ||
      !red_monkey_mpg_descriptor_sha256(descriptor, length, digest)) {
    return false;
  }
  for (size_t i = 0; i < sizeof(digest); ++i) {
    output[i * 2] = hex[digest[i] >> 4];
    output[i * 2 + 1] = hex[digest[i] & 0x0f];
  }
  output[64] = '\0';
  return true;
}

bool red_monkey_mpg_descriptor_matches_sha256(const uint8_t* descriptor,
                                       size_t length,
                                       const char* expected_hex) {
  if (expected_hex == NULL || strlen(expected_hex) != 64) return false;
  uint8_t expected[32];
  for (size_t i = 0; i < sizeof(expected); ++i) {
    const int high = hex_nibble(expected_hex[i * 2]);
    const int low = hex_nibble(expected_hex[i * 2 + 1]);
    if (high < 0 || low < 0) return false;
    expected[i] = (uint8_t)((high << 4) | low);
  }
  uint8_t actual[32];
  if (!red_monkey_mpg_descriptor_sha256(descriptor, length, actual)) return false;
  unsigned int different = 0;
  for (size_t i = 0; i < sizeof(actual); ++i) {
    different |= (unsigned int)(actual[i] ^ expected[i]);
  }
  return different == 0;
}
