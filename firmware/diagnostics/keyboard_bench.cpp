// Bounded USB keyboard test for a normal computer. Do not connect this image
// to a machine: it deliberately emits all six jog keys and one rapid key chord.

#include <array>
#include <cstddef>
#include <cstdint>

#include "red_monkey_mpg/masso_keyboard.hpp"
#include "pico/stdlib.h"
#include "tusb.h"

namespace {

using red_monkey_mpg::Axis;
using red_monkey_mpg::Direction;
using red_monkey_mpg::JogCommand;
using red_monkey_mpg::JogRate;
using red_monkey_mpg::OutputFrame;

constexpr std::uint32_t kStartupDelayMs = 8000;
constexpr std::uint32_t kReportIntervalMs = 600;

constexpr std::array<JogCommand, 7> kSequence{{
    {Axis::x, Direction::negative, JogRate::normal},
    {Axis::x, Direction::positive, JogRate::normal},
    {Axis::y, Direction::negative, JogRate::normal},
    {Axis::y, Direction::positive, JogRate::normal},
    {Axis::z, Direction::negative, JogRate::normal},
    {Axis::z, Direction::positive, JogRate::normal},
    {Axis::x, Direction::positive, JogRate::rapid},
}};

std::uint32_t mounted_at_ms{};
std::uint32_t last_report_ms{};
std::size_t command_index{};
bool key_is_down{};
bool finished{};

void send_release() { tud_hid_keyboard_report(0, 0, nullptr); }

void run_bounded_test(std::uint32_t now_ms) {
  if (finished || !tud_mounted() || !tud_hid_ready()) return;
  if (now_ms - mounted_at_ms < kStartupDelayMs) return;
  if (now_ms - last_report_ms < kReportIntervalMs) return;

  last_report_ms = now_ms;
  if (key_is_down) {
    send_release();
    key_is_down = false;
    ++command_index;
    if (command_index == kSequence.size()) finished = true;
    return;
  }

  OutputFrame frame{};
  frame.release_all = false;
  frame.has_jog = true;
  frame.jog = kSequence[command_index];
  const auto report = red_monkey_mpg::masso_g3_keyboard_report(frame);
  tud_hid_keyboard_report(0, report[0], report.data() + 2);
  key_is_down = true;
}

}  // namespace

extern "C" void tud_mount_cb(void) {
  if (mounted_at_ms == 0) mounted_at_ms = to_ms_since_boot(get_absolute_time());
}

extern "C" void tud_umount_cb(void) {
  // Never restart the sequence after a USB reset within the same boot.
  key_is_down = false;
}

extern "C" uint16_t tud_hid_get_report_cb(uint8_t instance,
                                           uint8_t report_id,
                                           hid_report_type_t report_type,
                                           uint8_t* buffer, uint16_t reqlen) {
  (void)instance;
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)reqlen;
  return 0;
}

extern "C" void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                                       hid_report_type_t report_type,
                                       uint8_t const* buffer,
                                       uint16_t bufsize) {
  (void)instance;
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)bufsize;
}

int main() {
  tusb_rhport_init_t device_init{
      .role = TUSB_ROLE_DEVICE,
      .speed = TUSB_SPEED_AUTO,
  };
  tusb_init(0, &device_init);
  while (true) {
    tud_task();
    run_bounded_test(to_ms_since_boot(get_absolute_time()));
    tight_loop_contents();
  }
}
