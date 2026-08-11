#pragma once

#include <cstdint>

#include "red_monkey_mpg/production_config.hpp"

namespace red_monkey_mpg {

// Uses two independent BTstack TLV tags. A new record is written and read back
// before it can supersede the previous valid record.
class PersistentConfig {
 public:
  bool load(ProductionMapping& mapping);
  bool store(const ProductionMapping& mapping);
  bool store_controller_address(const ProductionMapping& mapping,
                                const char* address);
  const char* controller_address() const {
    return controller_address_[0] == '\0' ? nullptr : controller_address_;
  }
  std::uint32_t sequence() const { return sequence_; }

 private:
  std::uint32_t sequence_{};
  std::uint32_t active_tag_{};
  char controller_address_[18]{};
};

}  // namespace red_monkey_mpg
