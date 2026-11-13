#pragma once

#include <cstdint>

#ifndef MESHFLOW_M9_SPACE_HOLD_MS
#define MESHFLOW_M9_SPACE_HOLD_MS 700U
#endif

#ifndef MESHFLOW_M9_CENTER_DOUBLE_CLICK_MIN_MS
#define MESHFLOW_M9_CENTER_DOUBLE_CLICK_MIN_MS 80U
#endif

#ifndef MESHFLOW_M9_CENTER_DOUBLE_CLICK_MAX_MS
#define MESHFLOW_M9_CENTER_DOUBLE_CLICK_MAX_MS 700U
#endif

// Timing policy shared by the M9 keyboard driver and host-side tests.
class M9KeyboardLockGesture
{
  public:
    enum class SpaceAction { None, Wait, EmitSpace, Lock };
    enum class CenterInputAction { DispatchNormally, Suppress, Unlock };

    CenterInputAction processCenterInput(uint32_t &rawKey, uint32_t noKey, bool keyHeld, uint32_t nowMs,
                                         bool manuallyLocked)
    {
        if (centerKeyDown && rawKey == noKey && !keyHeld)
            centerReleased();
        if (centerKeyDown && rawKey == UNLOCK_GESTURE_KEY)
            rawKey = noKey;

        if (!manuallyLocked)
            return CenterInputAction::DispatchNormally;

        if (rawKey == UNLOCK_GESTURE_KEY) {
            if (centerPressed(nowMs)) {
                rawKey = noKey;
                return CenterInputAction::Unlock;
            }
            return CenterInputAction::Suppress;
        }
        if (rawKey != noKey)
            otherKeyPressed();
        return CenterInputAction::Suppress;
    }

    void beginSpace(uint32_t nowMs)
    {
        if (!spacePending) {
            spacePending = true;
            spaceStartedAt = nowMs;
            centerTapArmed = false;
        }
    }

    SpaceAction updateSpace(uint32_t nowMs, bool keyHeld)
    {
        if (!spacePending)
            return SpaceAction::None;
        // The keyboard reports Space on key-down, so the active-high shared
        // interrupt line is the only release/hold signal. A delayed poll must
        // not turn a tap into a lock after the key has already been released.
        if (!keyHeld) {
            spacePending = false;
            return SpaceAction::EmitSpace;
        }
        if ((uint32_t)(nowMs - spaceStartedAt) >= SPACE_HOLD_MS) {
            spacePending = false;
            centerTapArmed = false;
            return SpaceAction::Lock;
        }
        return SpaceAction::Wait;
    }

    // A distinct key event means the deferred Space press ended, even if the
    // shared keyboard interrupt line is still active for that other key.
    SpaceAction endSpace()
    {
        if (!spacePending)
            return SpaceAction::None;
        spacePending = false;
        centerTapArmed = false;
        return SpaceAction::EmitSpace;
    }

    bool centerPressed(uint32_t nowMs)
    {
        if (centerKeyDown)
            return false; // Repeats during one long press are not a second click.
        centerKeyDown = true;
        if (centerTapArmed) {
            const uint32_t elapsed = nowMs - centerTapAt;
            if (elapsed < CENTER_DOUBLE_MIN_MS)
                return false; // Ignore bounce without moving the first-click time.
            if (elapsed <= CENTER_DOUBLE_MAX_MS) {
                centerTapArmed = false;
                return true;
            }
        }
        centerTapAt = nowMs;
        centerTapArmed = true;
        return false;
    }

    void centerReleased() { centerKeyDown = false; }
    void otherKeyPressed() { centerTapArmed = false; }
    void reset()
    {
        spacePending = false;
        centerTapArmed = false;
        centerKeyDown = false;
    }
    bool isSpacePending() const { return spacePending; }
    bool isCenterDown() const { return centerKeyDown; }

    static constexpr uint32_t SPACE_HOLD_MS = MESHFLOW_M9_SPACE_HOLD_MS;
    static constexpr uint32_t CENTER_DOUBLE_MIN_MS = MESHFLOW_M9_CENTER_DOUBLE_CLICK_MIN_MS;
    static constexpr uint32_t CENTER_DOUBLE_MAX_MS = MESHFLOW_M9_CENTER_DOUBLE_CLICK_MAX_MS;
    static constexpr uint32_t UNLOCK_GESTURE_KEY = 0x0D;
    static_assert(CENTER_DOUBLE_MIN_MS <= CENTER_DOUBLE_MAX_MS,
                  "M9 double-click minimum must not exceed its maximum");

  private:
    bool spacePending = false;
    uint32_t spaceStartedAt = 0;
    bool centerTapArmed = false;
    uint32_t centerTapAt = 0;
    bool centerKeyDown = false;
};
