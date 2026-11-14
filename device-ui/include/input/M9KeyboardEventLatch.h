#pragma once

#include <atomic>
#include <cstdint>

// The TM9 reports printable characters and these dedicated key codes.  Other
// bytes (including controller status and I2C errors) must not wake the UI.
inline bool m9IsWakeKey(uint32_t key)
{
    return (key >= 0x20 && key <= 0x7e) || key == 0x08 || key == 0x0d ||
           (key >= 0x81 && key <= 0x87) || key == 0x89 || key == 0xa3 ||
           (key >= 0xb4 && key <= 0xb7);
}

// ISR-to-LVGL handoff for the STC8H key interrupt. exchange(false) makes
// claiming a pending read atomic, so an interrupt arriving during the I2C
// transaction remains pending for the next input scan.
class M9KeyboardEventLatch
{
  public:
    void notify(void) { pending.store(true, std::memory_order_release); }
    bool take(void) { return pending.exchange(false, std::memory_order_acq_rel); }

  private:
    std::atomic<bool> pending{false};
};
