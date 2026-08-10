#pragma once

#include <cstddef>
#include <cstdint>

#include "openmpg/persistent_config.hpp"

namespace openmpg {

struct ConfigServiceHooks {
  void (*start_scan)(){};
  bool (*select_candidate)(const char* address){};
  bool (*confirm_pairing)(){};
  void (*cancel_pairing)(){};
};

class ConfigService {
 public:
  ConfigService(ProductionMapping& mapping, PersistentConfig& storage,
                ConfigServiceHooks hooks = {});
  void set_identity(const char* serial, const char* firmware,
                    const char* controller, const char* address);
  void service();
  bool active() const { return active_; }
  void emit_scan_result(const char* name, const char* address, int rssi);
  void emit_pairing_state(const char* state);

 private:
  void handle_line(char* line);
  void respond(bool ok, const char* body);
  bool write_line(const char* line);
  void service_tx();

  ProductionMapping& mapping_;
  PersistentConfig& storage_;
  ConfigServiceHooks hooks_;
  const char* serial_{"unknown"};
  const char* firmware_{"unknown"};
  const char* controller_{nullptr};
  const char* address_{nullptr};
  char input_[1025]{};
  char tx_[4096]{};
  std::size_t tx_size_{};
  std::size_t tx_offset_{};
  std::uint16_t input_size_{};
  bool active_{};
  bool overflow_{};
};

}  // namespace openmpg
