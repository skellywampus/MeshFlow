#pragma once
#include <cstddef>
#include <cstdint>
namespace m9power {
// The ESP32 ROM nano printf does not support the ll length modifier. Convert
// counters before passing them to printf so later %s arguments stay aligned.
inline const char *formatDecimal(uint64_t value, char (&text)[21]) {
    char *end = text + sizeof(text) - 1;
    *end = '\0';
    do {
        *--end = char('0' + value % 10);
        value /= 10;
    } while (value);
    return end;
}
struct Point { uint32_t hour; uint16_t mv; uint8_t pct, valid; };
struct Record {
    uint32_t magic = 0x50575231, sequence = 0;
    uint64_t cpu80 = 0, cpu240 = 0, cpuOther = 0, on = 0, off = 0, battery = 0;
    uint32_t wakes = 0;
    Point history[168]{};
    uint32_t crc = 0;
};
inline uint32_t checksum(const Record &r) {
    const auto *p = reinterpret_cast<const uint8_t *>(&r);
    uint32_t c = 2166136261U;
    for (size_t i = 0; i < offsetof(Record, crc); ++i) c = (c ^ p[i]) * 16777619U;
    return c;
}
inline void advance(Record &r, uint64_t elapsed, uint32_t mhz, bool screenOn, bool battery) {
    (mhz == 80 ? r.cpu80 : mhz == 240 ? r.cpu240 : r.cpuOther) += elapsed;
    (screenOn ? r.on : r.off) += elapsed;
    if (battery) r.battery += elapsed;
}
}
