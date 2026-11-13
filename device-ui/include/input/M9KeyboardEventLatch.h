#pragma once

#include <atomic>

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
