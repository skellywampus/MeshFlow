#pragma once

#include <cstdint>

class M9DoubleHomeGesture
{
  public:
    bool press(uint32_t now)
    {
        if (pending && uint32_t(now - started) <= 500) { pending = false; return true; }
        started = now; pending = true; return false;
    }
    void reset() { pending = false; }
  private:
    uint32_t started = 0;
    bool pending = false;
};

// Keep the M9's explicit manual lock separate from the view's timeout/legacy
// blank-screen state. A timed-out view must be restored before saving it as
// the screen to resume after the manual unlock gesture.
class M9ScreenLockPolicy
{
  public:
    static constexpr uint8_t UNLOCK_GESTURE_KEY = 0x0D;

    // A manual lock may accept only its Center/Enter gesture. All regular key
    // delivery must stop until that gesture succeeds.
    static constexpr bool shouldDispatchNormalInput(bool manuallyLocked)
    {
        return !manuallyLocked;
    }

    static constexpr bool isUnlockGestureKey(uint32_t rawKey)
    {
        return rawKey == UNLOCK_GESTURE_KEY;
    }

    static constexpr bool shouldRestoreBeforeManualLock(bool manuallyLocked, bool viewLocked)
    {
        return !manuallyLocked && viewLocked;
    }

    // The blank-screen button is the ordinary timeout wake affordance. It must
    // never stand in for the required Center double-click during a manual lock.
    static constexpr bool shouldAcceptTimeoutWake(bool manuallyLocked)
    {
        return !manuallyLocked;
    }

    // Automatic wake sources (display toggle, touch/button activity, or timeout
    // restoration) must not bypass the explicit Center double-click unlock.
    static constexpr bool shouldAllowAutomaticDisplayWake(bool manuallyLocked)
    {
        return !manuallyLocked;
    }

    // Incoming messages still update history and unread counts while locked,
    // but their UI banner must not steal focus or wake display activity.
    static constexpr bool shouldPresentIncomingBanner(bool manuallyLocked)
    {
        return !manuallyLocked;
    }
};
