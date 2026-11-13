#pragma once
#include <cstdint>
struct M9SleepStats {
    uint64_t entries, failures, successCallUs, eligibleCallUs, eligibleUs, pendingSkips, timerWakes, gpioWakes, otherWakes;
    uint32_t initialized, error, screenOff, gpsOn, wifiOn, usbOn, busUsers, wifiSleepReady;
};
extern "C" void meshflow_m9_pm_ui(bool screenOff, bool wifiOn);
extern "C" void meshflow_m9_pm_gps(bool active);
extern "C" void meshflow_m9_pm_radio_arm(bool active);
extern "C" bool meshflow_m9_pm_claim_radio();
extern "C" bool meshflow_m9_pm_bus_acquire();
extern "C" void meshflow_m9_pm_bus_release(bool held);
extern "C" void meshflow_m9_pm_snapshot(M9SleepStats *out);
extern "C" void meshflow_m9_pm_power_sample();
extern "C" void meshflow_m9_pm_audio(bool active);
