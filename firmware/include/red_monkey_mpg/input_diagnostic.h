#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void red_monkey_mpg_diagnose_lite2_report(const uint8_t* report, uint16_t length,
                                   uint32_t now_ms);

#ifdef __cplusplus
}
#endif
