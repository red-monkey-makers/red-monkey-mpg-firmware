#include "openmpg/persistent_config.hpp"

#include <array>
#include <cstdio>
#include <cstring>

#include "btstack_tlv.h"

namespace openmpg {
namespace {

constexpr std::uint32_t kTagA = 0x4f4d5041;  // OMPA
constexpr std::uint32_t kTagB = 0x4f4d5042;  // OMPB
constexpr std::uint32_t kMagic = 0x4f4d5047;  // OMPG
constexpr std::uint16_t kSchema = 1;

#pragma pack(push, 1)
struct Record {
  std::uint32_t magic;
  std::uint16_t schema;
  std::uint16_t size;
  std::uint32_t sequence;
  std::uint32_t deadman_mask;
  std::uint32_t continuous_mask;
  std::uint32_t precision_mask;
  std::uint32_t resolution_mask;
  std::uint8_t face[4];
  std::uint8_t controller_address[6];
  std::uint8_t controller_valid;
  // This byte was zero-filled reserved storage in schema 1. Profile zero is
  // therefore the backward-compatible MASSO profile for existing records.
  std::uint8_t cnc_profile;
  std::uint32_t crc;
};
#pragma pack(pop)

static_assert(sizeof(Record) == 44, "Persistent record layout changed");

std::uint32_t crc32(const std::uint8_t* data, std::size_t size) {
  std::uint32_t crc = 0xffffffffu;
  for (std::size_t i = 0; i < size; ++i) {
    crc ^= data[i];
    for (int bit = 0; bit < 8; ++bit) {
      crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
    }
  }
  return ~crc;
}

bool address_bytes_valid(const std::uint8_t address[6]) {
  bool any_nonzero = false;
  bool any_not_ff = false;
  for (int i = 0; i < 6; ++i) {
    any_nonzero = any_nonzero || address[i] != 0;
    any_not_ff = any_not_ff || address[i] != 0xff;
  }
  return any_nonzero && any_not_ff;
}

bool parse_address(const char* text, std::uint8_t output[6]) {
  if (text == nullptr) return false;
  unsigned int bytes[6]{};
  char tail{};
  if (std::sscanf(text, "%2x:%2x:%2x:%2x:%2x:%2x%c", &bytes[0], &bytes[1],
                  &bytes[2], &bytes[3], &bytes[4], &bytes[5], &tail) != 6) {
    return false;
  }
  for (int i = 0; i < 6; ++i) output[i] = static_cast<std::uint8_t>(bytes[i]);
  return address_bytes_valid(output);
}

Record encode(const ProductionMapping& mapping, std::uint32_t sequence,
              const char* address) {
  Record record{};
  record.magic = kMagic;
  record.schema = kSchema;
  record.size = sizeof(Record);
  record.sequence = sequence;
  record.deadman_mask = mapping.deadman_mask;
  record.continuous_mask = mapping.continuous_mask;
  record.precision_mask = mapping.precision_mask;
  record.resolution_mask = mapping.resolution_mask;
  record.face[0] = static_cast<std::uint8_t>(mapping.face_a);
  record.face[1] = static_cast<std::uint8_t>(mapping.face_b);
  record.face[2] = static_cast<std::uint8_t>(mapping.face_x);
  record.face[3] = static_cast<std::uint8_t>(mapping.face_y);
  record.cnc_profile = static_cast<std::uint8_t>(mapping.cnc_profile);
  record.controller_valid = parse_address(address, record.controller_address);
  record.crc = crc32(reinterpret_cast<const std::uint8_t*>(&record),
                     sizeof(Record) - sizeof(record.crc));
  return record;
}

bool decode(const Record& record, ProductionMapping& mapping) {
  if (record.magic != kMagic || record.schema != kSchema ||
      record.size != sizeof(Record) ||
      record.controller_valid > 1 ||
      (record.controller_valid != 0 &&
       !address_bytes_valid(record.controller_address)) ||
      record.crc != crc32(reinterpret_cast<const std::uint8_t*>(&record),
                          sizeof(Record) - sizeof(record.crc))) {
    return false;
  }
  ProductionMapping candidate{};
  candidate.cnc_profile =
      static_cast<CncControllerProfileId>(record.cnc_profile);
  candidate.deadman_mask = record.deadman_mask;
  candidate.continuous_mask = record.continuous_mask;
  candidate.precision_mask = record.precision_mask;
  candidate.resolution_mask = record.resolution_mask;
  candidate.face_a = static_cast<ButtonAction>(record.face[0]);
  candidate.face_b = static_cast<ButtonAction>(record.face[1]);
  candidate.face_x = static_cast<ButtonAction>(record.face[2]);
  candidate.face_y = static_cast<ButtonAction>(record.face[3]);
  if (validate_production_mapping(candidate) != ConfigValidation::valid) {
    return false;
  }
  mapping = candidate;
  return true;
}

bool newer(std::uint32_t left, std::uint32_t right) {
  return static_cast<std::int32_t>(left - right) > 0;
}

bool read_record(const btstack_tlv_t* tlv, void* context, std::uint32_t tag,
                 Record& record, ProductionMapping& mapping) {
  const int size = tlv->get_tag(context, tag,
                                reinterpret_cast<std::uint8_t*>(&record),
                                sizeof(record));
  return size == static_cast<int>(sizeof(record)) && decode(record, mapping);
}

}  // namespace

bool PersistentConfig::load(ProductionMapping& mapping) {
  // A failed reload must never leave previously loaded identity or mapping in
  // memory. Start from the compiled safe default before touching flash.
  mapping = ProductionMapping{};
  controller_address_[0] = '\0';
  sequence_ = 0;
  active_tag_ = 0;
  const btstack_tlv_t* tlv = nullptr;
  void* context = nullptr;
  btstack_tlv_get_instance(&tlv, &context);
  if (tlv == nullptr) return false;

  Record a{}, b{};
  ProductionMapping mapping_a{}, mapping_b{};
  const bool valid_a = read_record(tlv, context, kTagA, a, mapping_a);
  const bool valid_b = read_record(tlv, context, kTagB, b, mapping_b);
  if (!valid_a && !valid_b) return false;
  if (valid_b && (!valid_a || newer(b.sequence, a.sequence))) {
    mapping = mapping_b;
    sequence_ = b.sequence;
    active_tag_ = kTagB;
    if (b.controller_valid) {
      std::snprintf(controller_address_, sizeof(controller_address_),
                    "%02X:%02X:%02X:%02X:%02X:%02X",
                    b.controller_address[0], b.controller_address[1],
                    b.controller_address[2], b.controller_address[3],
                    b.controller_address[4], b.controller_address[5]);
    }
  } else {
    mapping = mapping_a;
    sequence_ = a.sequence;
    active_tag_ = kTagA;
    if (a.controller_valid) {
      std::snprintf(controller_address_, sizeof(controller_address_),
                    "%02X:%02X:%02X:%02X:%02X:%02X",
                    a.controller_address[0], a.controller_address[1],
                    a.controller_address[2], a.controller_address[3],
                    a.controller_address[4], a.controller_address[5]);
    }
  }
  return true;
}

bool PersistentConfig::store(const ProductionMapping& mapping) {
  if (validate_production_mapping(mapping) != ConfigValidation::valid) {
    return false;
  }
  const btstack_tlv_t* tlv = nullptr;
  void* context = nullptr;
  btstack_tlv_get_instance(&tlv, &context);
  if (tlv == nullptr) return false;

  const std::uint32_t target = active_tag_ == kTagA ? kTagB : kTagA;
  const Record record = encode(mapping, sequence_ + 1, controller_address());
  if (tlv->store_tag(context, target,
                     reinterpret_cast<const std::uint8_t*>(&record),
                     sizeof(record)) != 0) {
    return false;
  }
  Record verify{};
  ProductionMapping decoded{};
  if (!read_record(tlv, context, target, verify, decoded) ||
      std::memcmp(&record, &verify, sizeof(record)) != 0) {
    // Do not leave a candidate record behind after reporting failure. A
    // transient read fault could otherwise make a valid-looking new record
    // appear only after reboot, contradicting the failed setup transaction.
    if (tlv->delete_tag != nullptr) tlv->delete_tag(context, target);
    return false;
  }
  sequence_ = record.sequence;
  active_tag_ = target;
  return true;
}

bool PersistentConfig::store_controller_address(const ProductionMapping& mapping,
                                                const char* address) {
  std::uint8_t parsed[6]{};
  if (!parse_address(address, parsed)) return false;
  char previous[sizeof(controller_address_)]{};
  std::memcpy(previous, controller_address_, sizeof(previous));
  std::snprintf(controller_address_, sizeof(controller_address_),
                "%02X:%02X:%02X:%02X:%02X:%02X", parsed[0], parsed[1],
                parsed[2], parsed[3], parsed[4], parsed[5]);
  if (store(mapping)) return true;
  std::memcpy(controller_address_, previous, sizeof(previous));
  return false;
}

}  // namespace openmpg
