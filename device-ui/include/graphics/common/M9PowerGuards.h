#pragma once
#include <cstdint>

struct M9WakePinState {
    uint32_t type, interrupts;
    bool wake;
};

// Backend owns the GPIO register/API details. Keep the same restoration path
// for completed sleep, rejected sleep, and partial wake-configuration failure.
class M9PowerWakeGuard {
    M9WakePinState saved{};
    bool active = false, poweredBefore = false, changed = false, failed = false;
  public:
    template <typename GPIO> bool arm(GPIO &gpio) {
        if (active) return false;
        saved = gpio.snapshot();
        poweredBefore = gpio.powered();
        active = true;
        gpio.mask();
        if (gpio.armWake()) return true;
        restore(gpio);
        return false;
    }
    template <typename GPIO> bool restore(GPIO &gpio) {
        if (!active) return false;
        gpio.mask();
        if (!gpio.restore(saved)) failed = true;
        active = false;
        changed = changed || poweredBefore != gpio.powered();
        return changed;
    }
    bool takeChanged() { bool result = changed; changed = false; return result; }
    bool faulted() const { return failed; }
};

// A playback owner acquires once, even when a nagging song restarts. Other
// owners may overlap; the PM implementation separately reference-counts them.
class M9PlaybackLease {
    bool held = false;
  public:
    template <typename Acquire> void begin(Acquire acquire) {
        if (!held) { acquire(); held = true; }
    }
    template <typename Release> void end(Release release) {
        if (held) { held = false; release(); }
    }
    bool active() const { return held; }
};
