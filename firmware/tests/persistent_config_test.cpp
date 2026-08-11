#include "red_monkey_mpg/persistent_config.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>

#include "btstack_tlv.h"

namespace {

struct Slot {
  std::uint32_t tag{};
  std::array<std::uint8_t, 64> bytes{};
  std::uint32_t size{};
  bool valid{};
};

struct FakeTlv {
  std::array<Slot, 2> slots{};
  bool fail_store{};
  bool corrupt_after_store{};
};

Slot* find_slot(FakeTlv& state, std::uint32_t tag) {
  for (auto& slot : state.slots) {
    if (slot.valid && slot.tag == tag) return &slot;
  }
  return nullptr;
}

int get_tag(void* context, std::uint32_t tag, std::uint8_t* buffer,
            std::uint32_t buffer_size) {
  auto& state = *static_cast<FakeTlv*>(context);
  const Slot* slot = find_slot(state, tag);
  if (slot == nullptr) return 0;
  const auto amount = slot->size < buffer_size ? slot->size : buffer_size;
  std::memcpy(buffer, slot->bytes.data(), amount);
  return static_cast<int>(slot->size);
}

int store_tag(void* context, std::uint32_t tag, const std::uint8_t* data,
              std::uint32_t data_size) {
  auto& state = *static_cast<FakeTlv*>(context);
  if (state.fail_store || data_size > state.slots[0].bytes.size()) return -1;
  Slot* slot = find_slot(state, tag);
  if (slot == nullptr) {
    for (auto& candidate : state.slots) {
      if (!candidate.valid) {
        slot = &candidate;
        break;
      }
    }
  }
  if (slot == nullptr) slot = &state.slots[0];
  slot->tag = tag;
  slot->size = data_size;
  slot->valid = true;
  std::memcpy(slot->bytes.data(), data, data_size);
  if (state.corrupt_after_store && data_size > 0) slot->bytes[0] ^= 0x01;
  return 0;
}

void delete_tag(void* context, std::uint32_t tag) {
  auto& state = *static_cast<FakeTlv*>(context);
  if (Slot* slot = find_slot(state, tag)) slot->valid = false;
}

FakeTlv fake_tlv{};
const btstack_tlv_t implementation = {get_tag, store_tag, delete_tag};

}  // namespace

extern "C" void btstack_tlv_get_instance(
    const btstack_tlv_t** output_implementation, void** output_context) {
  *output_implementation = &implementation;
  *output_context = &fake_tlv;
}

int main() {
  using namespace red_monkey_mpg;
  ProductionMapping mapping{};
  PersistentConfig storage{};
  assert(!storage.load(mapping));
  assert(storage.sequence() == 0);

  assert(storage.store(mapping));
  assert(storage.sequence() == 1);
  PersistentConfig reloaded{};
  ProductionMapping decoded{};
  assert(reloaded.load(decoded));
  assert(reloaded.sequence() == 1);
  assert(reloaded.controller_address() == nullptr);
  assert(decoded.deadman_mask == mapping.deadman_mask);
  assert(decoded.cnc_profile == mapping.cnc_profile);

  assert(reloaded.store_controller_address(decoded, "00:00:5e:00:53:01"));
  assert(reloaded.sequence() == 2);
  PersistentConfig bonded{};
  assert(bonded.load(decoded));
  assert(std::strcmp(bonded.controller_address(), "00:00:5E:00:53:01") == 0);

  ProductionMapping changed = decoded;
  changed.deadman_mask = 0x00040;  // L1
  assert(bonded.store(changed));
  assert(bonded.sequence() == 3);

  // Corrupt the newest alternating record. Load must fall back to the previous
  // complete CRC-valid record rather than accepting partially written state.
  constexpr std::uint32_t kTagA = 0x4f4d5041;
  Slot* newest = find_slot(fake_tlv, kTagA);
  assert(newest != nullptr);
  newest->bytes[12] ^= 0x80;
  PersistentConfig fallback{};
  ProductionMapping fallback_mapping{};
  assert(fallback.load(fallback_mapping));
  assert(fallback.sequence() == 2);
  assert(fallback_mapping.deadman_mask == mapping.deadman_mask);
  assert(std::strcmp(fallback.controller_address(), "00:00:5E:00:53:01") == 0);

  fake_tlv.fail_store = true;
  assert(!fallback.store(changed));
  assert(fallback.sequence() == 2);
  fake_tlv.fail_store = false;

  fake_tlv.corrupt_after_store = true;
  assert(!fallback.store(changed));
  assert(fallback.sequence() == 2);
  assert(find_slot(fake_tlv, kTagA) == nullptr);
  fake_tlv.corrupt_after_store = false;
  assert(fallback.store(changed));
  assert(fallback.sequence() == 3);

  ProductionMapping invalid = changed;
  invalid.deadman_mask = invalid.continuous_mask;
  assert(!fallback.store(invalid));
  assert(fallback.sequence() == 3);
  assert(!fallback.store_controller_address(changed, "not-an-address"));
  assert(!fallback.store_controller_address(changed, "00:00:5E:00:53:01x"));
  assert(!fallback.store_controller_address(changed, "00:00:00:00:00:00"));
  assert(!fallback.store_controller_address(changed, "FF:FF:FF:FF:FF:FF"));

  // Reusing a PersistentConfig object must clear a stale address when the
  // selected valid record has no controller.
  fake_tlv = {};
  PersistentConfig unpaired_writer{};
  assert(unpaired_writer.store(mapping));
  assert(fallback.load(decoded));
  assert(fallback.controller_address() == nullptr);

  // If all records later become unreadable, reusing the same storage object
  // must clear its old identity and restore the compiled mapping defaults.
  decoded.deadman_mask = 0x00040;
  fake_tlv = {};
  assert(!fallback.load(decoded));
  assert(fallback.controller_address() == nullptr);
  assert(fallback.sequence() == 0);
  assert(decoded.deadman_mask == ProductionMapping{}.deadman_mask);
}
