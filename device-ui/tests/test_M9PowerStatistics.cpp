#include "graphics/common/M9PowerCounters.h"
#include <doctest/doctest.h>
#include <cstring>
#include <limits>

TEST_CASE("Power counter text supports the full uint64 range without printf length modifiers") {
    char text[21];
    CHECK(std::strcmp(m9power::formatDecimal(0, text), "0") == 0);
    CHECK(std::strcmp(m9power::formatDecimal(9, text), "9") == 0);
    CHECK(std::strcmp(m9power::formatDecimal(10, text), "10") == 0);
    CHECK(std::strcmp(m9power::formatDecimal(4294967295ULL, text), "4294967295") == 0);
    CHECK(std::strcmp(m9power::formatDecimal(4294967296ULL, text), "4294967296") == 0);
    CHECK(std::strcmp(m9power::formatDecimal(std::numeric_limits<uint64_t>::max(), text), "18446744073709551615") == 0);
}

// Exercise the actual counter/checkpoint helpers: transitions must not attribute
// elapsed time to the next state, and a damaged checkpoint must fail integrity.
TEST_CASE("Power counters retain state durations without counting downtime") {
    m9power::Record r;
    m9power::advance(r, 2000000, 240, true, false);
    m9power::advance(r, 3000000, 80, false, true);
    m9power::advance(r, 4000000, 160, false, false);
    CHECK(r.cpu240 == 2000000);
    CHECK(r.cpu80 == 3000000);
    CHECK(r.cpuOther == 4000000);
    CHECK(r.on == 2000000);
    CHECK(r.off == 7000000);
    CHECK(r.battery == 3000000);
    CHECK(r.on + r.off == r.cpu80 + r.cpu240 + r.cpuOther);
    m9power::advance(r, 0, 80, true, true);
    CHECK(r.battery == 3000000);
}
TEST_CASE("Power checkpoint roundtrip and corruption detection") {
    m9power::Record r;
    r.sequence = 12;
    r.history[10] = {1000, 3890, 62, 1};
    r.cpu80 = 10000000000ULL;
    r.crc = m9power::checksum(r);
    m9power::Record loaded;
    memcpy(&loaded, &r, sizeof(r));
    CHECK(loaded.crc == m9power::checksum(loaded));
    CHECK(loaded.cpu80 == 10000000000ULL);
    CHECK(loaded.history[10].pct == 62);
    loaded.history[10].mv++;
    CHECK(loaded.crc != m9power::checksum(loaded));
    loaded = r;
    loaded.sequence++;
    CHECK(loaded.crc != m9power::checksum(loaded));
    CHECK(sizeof(r) < 2048);
}
