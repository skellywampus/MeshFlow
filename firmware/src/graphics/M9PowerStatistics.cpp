#if defined(ELECROW_ThinkNode_M9) && HAS_TFT
#include "configuration.h"
#include "graphics/common/M9PowerStatistics.h"
#include "graphics/common/M9PowerCounters.h"
#ifdef MESHFLOW_M9_AUTO_LIGHT_SLEEP
#include "graphics/common/M9AutoLightSleep.h"
#endif
#include "PowerStatus.h"
#include "GPSStatus.h"
#include "FSCommon.h"
#include "airtime.h"
#include "mesh/RadioLibInterface.h"
#include "gps/RTC.h"
#include "main.h"
#include "nimble/NimbleBluetooth.h"
#include <WiFi.h>
#include <esp_timer.h>
#include <esp32-hal-cpu.h>
#include <cstdarg>
#include <algorithm>
#include <cstring>
#include <string>

namespace {
using m9power::Record;
using m9power::checksum;
Record data;
struct BatteryRun {
    uint32_t magic = 0x42525431, sequence = 0;
    uint64_t elapsed = 0;
    uint32_t observedExternal = 0;
    uint32_t crc = 0;
};
BatteryRun batteryRun;
uint32_t runChecksum(const BatteryRun &r) {
    const auto *p = reinterpret_cast<const uint8_t *>(&r);
    uint32_t hash = 2166136261U;
    for (size_t i = 0; i < offsetof(BatteryRun, crc); ++i) hash = (hash ^ p[i]) * 16777619U;
    return hash;
}
uint64_t last = 0, savedAt = 0, batteryStart = 0;
uint32_t frequency = 240;
bool initialized = false, on = true, batteryKnown = false, batteryRunning = false, storageOK = true;
int initialPercent = -1;
const char *paths[] = {"/powerstats-a.bin", "/powerstats-b.bin"};
const char *runPaths[] = {"/battery-run-a.bin", "/battery-run-b.bin"};
void saveBatteryRun() {
    if (FSCom.totalBytes() <= FSCom.usedBytes() + sizeof(BatteryRun) * 3) { storageOK = false; return; }
    ++batteryRun.sequence;
    batteryRun.crc = runChecksum(batteryRun);
    auto file = FSCom.open(runPaths[batteryRun.sequence % 2], FILE_O_WRITE);
    if (!file || file.write(reinterpret_cast<const uint8_t *>(&batteryRun), sizeof(batteryRun)) != sizeof(batteryRun))
        storageOK = false;
    if (file) { file.flush(); file.close(); }
}
void init() {
    if (initialized) return;
    initialized = true;
    for (const auto *path : paths) {
        Record candidate;
        auto file = FSCom.open(path, FILE_O_READ);
        if (file && file.size() == sizeof(candidate) && file.read(reinterpret_cast<uint8_t *>(&candidate), sizeof(candidate)) == sizeof(candidate) &&
            candidate.magic == data.magic && candidate.crc == checksum(candidate) && candidate.sequence >= data.sequence)
            data = candidate;
    }
    for (const auto *path : runPaths) {
        BatteryRun candidate;
        auto file = FSCom.open(path, FILE_O_READ);
        if (file && file.size() == sizeof(candidate) && file.read(reinterpret_cast<uint8_t *>(&candidate), sizeof(candidate)) == sizeof(candidate) &&
            candidate.magic == batteryRun.magic && candidate.crc == runChecksum(candidate) && candidate.sequence >= batteryRun.sequence)
            batteryRun = candidate;
    }
    last = savedAt = esp_timer_get_time();
}
void accrue() {
    init();
    uint64_t now = esp_timer_get_time(), dt = now - last;
    last = now;
    m9power::advance(data, dt, frequency, on, batteryKnown && batteryRunning);
    if (batteryKnown && batteryRunning) batteryRun.elapsed += dt;
}
void checkpoint() {
    if (last - savedAt < 3600000000ULL) return;
    savedAt = last;
    // No mount/format operation: use the already mounted native filesystem only.
    if (FSCom.totalBytes() <= FSCom.usedBytes() + sizeof(Record) * 3) { storageOK = false; return; }
    ++data.sequence;
    data.crc = checksum(data);
    auto file = FSCom.open(paths[data.sequence % 2], FILE_O_WRITE);
    storageOK = file && file.write(reinterpret_cast<const uint8_t *>(&data), sizeof(data)) == sizeof(data);
    if (file) { file.flush(); file.close(); }
    saveBatteryRun();
}
void append(std::string &s, const char *fmt, ...) {
    char text[320]; va_list args; va_start(args, fmt); vsnprintf(text, sizeof(text), fmt, args); va_end(args); s += text;
}
double hours(uint64_t us) { return us / 3600000000.0; }
}
extern "C" void meshflow_power_state(uint32_t mhz, bool screenOn) {
    accrue();
    if (!on && screenOn) ++data.wakes;
    on = screenOn; frequency = mhz;
}
extern "C" void meshflow_power_sample() {
#ifdef MESHFLOW_M9_AUTO_LIGHT_SLEEP
    meshflow_m9_pm_power_sample();
#endif
    accrue();
    const bool known = powerStatus && powerStatus->isInitialized() && powerStatus->getHasBattery() &&
        powerStatus->getExternalPowerStatus() != meshtastic::OptUnknown;
    const bool running = known && !powerStatus->getHasUSB() && !powerStatus->getIsCharging();
    int pct = powerStatus ? powerStatus->getBatteryChargePercent() : -1;
    const bool valid = powerStatus && powerStatus->isInitialized() && powerStatus->getHasBattery() && pct >= 0 && pct <= 100 && powerStatus->getBatteryVoltageMv() > 0;
    if (!running || !valid) { batteryStart = 0; initialPercent = -1; }
    else if (!batteryStart) { batteryStart = last; initialPercent = pct; }
    if (known && (!batteryKnown || batteryRunning != running)) {
        // External power resets the run, even if the battery is not fully charged.
        if (!running) { batteryRun.elapsed = 0; batteryRun.observedExternal = 1; }
        saveBatteryRun();
    }
    batteryKnown = known; batteryRunning = running;
    uint32_t epoch = getValidTime(RTCQualityDevice);
    if (epoch && valid) {
        uint32_t hour = epoch / 3600;
        data.history[hour % 168] = m9power::Point{hour, uint16_t(powerStatus->getBatteryVoltageMv()), uint8_t(pct), 1};
    }
    checkpoint();
}
extern "C" void meshflow_power_text(char *buffer, size_t capacity) {
    accrue();
    std::string s;
    append(s, "BATTERY (voltage estimate)\n");
    if (powerStatus && powerStatus->isInitialized() && powerStatus->getHasBattery() && powerStatus->getBatteryVoltageMv() > 0) {
        append(s, "%u%%  %.3f V\n", powerStatus->getBatteryChargePercent(), powerStatus->getBatteryVoltageMv()/1000.0);
        append(s, "External power: %s\nCharging: %s\n", powerStatus->getExternalPowerStatus() == meshtastic::OptUnknown ? "unavailable" : powerStatus->getHasUSB() ? "detected" : "not detected",
            powerStatus->getChargingStatus() == meshtastic::OptUnknown ? "unavailable" : powerStatus->getIsCharging() ? "yes" : "no");
    } else append(s, "Battery reading unavailable\n");
    if (!batteryKnown) append(s, "Time on battery: unavailable\n");
    else if (!batteryRunning) append(s, "Time on battery: on external power\n");
    else append(s, "Time on battery: %.2f h%s\n", hours(batteryRun.elapsed), batteryRun.observedExternal ? "" : " (partial)");
    append(s, "Uptime: %.2f h\n", hours(esp_timer_get_time()));
    if (batteryStart && last-batteryStart >= 1800000000ULL && initialPercent >= int(powerStatus->getBatteryChargePercent()))
        append(s, "Discharge estimate: %.2f %%/h\n", (initialPercent-powerStatus->getBatteryChargePercent())/hours(last-batteryStart));
    else append(s, "Discharge estimate: n/a (<30 min samples)\n");
    append(s, "Current: not measured\n\nCPU & DISPLAY\nCPU now: %u MHz\n80 MHz: %.2f h; 240 MHz: %.2f h\nOther CPU: %.2f h\n", getCpuFrequencyMhz(), hours(data.cpu80), hours(data.cpu240), hours(data.cpuOther));
    double total = double(data.on+data.off);
    append(s, "Screen on: %.2f h (%.1f%%)\nScreen off: %.2f h (%.1f%%)\nWake count: %u\nState: %s\n", hours(data.on), total ? 100*data.on/total : 0, hours(data.off), total ? 100*data.off/total : 0, unsigned(data.wakes), on ? "display active" : "panel asleep / refresh paused");
#ifdef MESHFLOW_M9_AUTO_LIGHT_SLEEP
    append(s,"CPU totals include sleep.\n");
    M9SleepStats sleep{}; meshflow_m9_pm_snapshot(&sleep);
    append(s, "\nAUTOMATIC LIGHT SLEEP (this boot)\nSDK init: %s; error %u\n", sleep.initialized ? "ready" : "unavailable",sleep.error);
    char counter1[21], counter2[21], counter3[21];
    append(s, "Successful entries: %s\nFailed sleep calls: %s\nPending-radio vetoes: %s\n",
        m9power::formatDecimal(sleep.entries, counter1), m9power::formatDecimal(sleep.failures, counter2),
        m9power::formatDecimal(sleep.pendingSkips, counter3));
    append(s, "Sleep-call time: %.3f h (incl. overhead)\nEligible off: %.3f h\n",hours(sleep.successCallUs),hours(sleep.eligibleUs));
    if(sleep.eligibleUs) append(s, "Eligible in sleep calls: %.1f%% (est. incl. overhead)\n",100.0*sleep.eligibleCallUs/sleep.eligibleUs);
    else append(s,"Eligible fraction: n/a\n");
    append(s, "Wakes: timer %s / GPIO %s\nOther %s\nPolicy block: %s%s%s%s\nSPI/SD users: %u\n",
        m9power::formatDecimal(sleep.timerWakes, counter1), m9power::formatDecimal(sleep.gpioWakes, counter2),
        m9power::formatDecimal(sleep.otherWakes, counter3),
        sleep.screenOff ? "" : "display active; ",sleep.gpsOn ? "GNSS powered; " : "",sleep.wifiOn && !sleep.wifiSleepReady ? "Wi-Fi sleep unavailable; " : "",sleep.usbOn ? "USB/external power; " : "",sleep.busUsers);
    append(s,"Serial: %s\nOther driver locks may block sleep.\n",moduleConfig.serial.enabled ? "sleep blocked" : "off");
#else
    append(s,"SoC automatic light sleep: disabled\n");
#endif
    append(s, "\nCONNECTIVITY\nGNSS: %s\nWi-Fi: %s\nBLE: %s\n", !gpsStatus ? "unavailable" : gpsStatus->getIsPowerSaving() ? "power saving" : gpsStatus->getHasLock() ? "fix" : gpsStatus->getIsConnected() ? "acquiring" : "not connected",
        WiFi.getMode() == WIFI_OFF ? "off" : WiFi.status() == WL_CONNECTED ? "connected" : "enabled / not connected",
        !config.bluetooth.enabled ? "disabled in config" : !nimbleBluetooth ? "unavailable" : nimbleBluetooth->isConnected() ? "connected" : nimbleBluetooth->isActive() ? "active / not connected" : "inactive");
    if (airTime) {
        append(s, "TX utilization (1h): %.2f%%\nChannel busy (60s): %.2f%%\n", airTime->utilizationTXPercent(), airTime->channelUtilizationPercent());
        uint32_t tx[AirTime::getPeriodsToLog()]{};
        if (airTime->airtimeReport(TX_LOG, tx, AirTime::getPeriodsToLog())) { uint64_t sum=0; for (auto ms:tx) sum+=ms; append(s,"TX airtime (retained <=8h): %.1f s\n",sum/1000.0); }
    } else append(s,"Airtime unavailable\n");
    if (auto *radio = RadioLibInterface::instance)
        append(s,"Packets this boot: TX %u / RX good %u\nRX bad %u / relay %u / dropped %u\n", unsigned(radio->txGood), unsigned(radio->rxGood), unsigned(radio->rxBad), unsigned(radio->txRelay), unsigned(radio->txDrop));
    else append(s,"Packet counters unavailable\n");
    uint32_t now = getValidTime(RTCQualityDevice), hour = now/3600;
    append(s,"\nBATTERY HISTORY (hourly)\nAge       Percent     Voltage\n");
    unsigned count=0;
    if (now) for (unsigned age=0; age<24; ++age) { const auto &p=data.history[(hour-age)%168]; if(p.valid && p.hour==hour-age) { append(s,"%2u h      %3u%%       %.3f V\n",age,p.pct,p.mv/1000.0); ++count; } }
    if (!count) append(s,"No clock/history yet\n");
    append(s,"\n7 DAYS (24h blocks, sample ranges)\nAge      Samples   %% range / V range\n");
    if (now) for (unsigned day=0;day<7;++day) { unsigned n=0,lo=100,hi=0,mvLo=65535,mvHi=0; for (unsigned age=day*24;age<(day+1)*24;++age) {const auto &p=data.history[(hour-age)%168]; if(p.valid && p.hour==hour-age) {++n;lo=std::min(lo,unsigned(p.pct));hi=std::max(hi,unsigned(p.pct));mvLo=std::min(mvLo,unsigned(p.mv));mvHi=std::max(mvHi,unsigned(p.mv));}} if(n) append(s,"%u d    %u/24    %u-%u%% / %.2f-%.2f V\n",day,n,lo,hi,mvLo/1000.0,mvHi/1000.0); }
    append(s,"\nOff/gaps excluded; totals checkpoint hourly.\nUp to 1h lost on reset.\nStorage: %s\nRefresh: 5s while visible.\n",storageOK ? "checkpoint enabled" : "unavailable / write failed");
    if(capacity) { strncpy(buffer,s.c_str(),capacity-1); buffer[capacity-1]=0; }
}
#endif
