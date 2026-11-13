#pragma once
#include "graphics/common/M9AutoLightSleep.h"
struct M9SleepConditions {
    bool ready, screenOff, gpsPowered, wifiActive, spiBusy, radioArmed, usbPowered=false, serialActive=false;
};
inline uint32_t m9SleepBlocks(const M9SleepConditions &c) {
    return (!c.ready ? 1u : 0u) | (!c.screenOff ? 2u : 0u) | (c.gpsPowered ? 4u : 0u) |
           (c.wifiActive ? 8u : 0u) | (c.spiBusy ? 16u : 0u) | (!c.radioArmed ? 32u : 0u) | (c.usbPowered ? 64u : 0u) | (c.serialActive ? 128u : 0u);
}
// Caller serializes this shared latch across normal ISR and PM wake replay.
inline bool m9ClaimRadioIrq(bool &armed) {
    if (!armed) return false;
    armed=false;
    return true;
}
// kind: 1 timer, 2 GPIO, 0 other. Only a successful SDK return is an entry.
inline void m9RecordSleepReturn(M9SleepStats &s, bool success, uint64_t elapsed, bool eligible, unsigned kind) {
    if (!success) {++s.failures;return;}
    ++s.entries;s.successCallUs+=elapsed;
    if(eligible)s.eligibleCallUs+=elapsed;
    if(kind==1)++s.timerWakes;
    else if(kind==2)++s.gpioWakes;
    else ++s.otherWakes;
}
// Preserve native readout dispatch; otherwise detach the old handler before
// waking a worker that may install the next handler on the other core.
template <typename Claim, typename Detach, typename Rearm, typename Readout, typename Notify>
inline void m9DispatchIdleWake(bool tx, bool fastReadout, Claim claim, Detach detach, Rearm rearm,
                              Readout readout, Notify notify) {
    if (!tx && fastReadout) {readout();return;}
    if (!claim()) return;
    detach();
    if (tx) rearm();
    notify(tx);
}
