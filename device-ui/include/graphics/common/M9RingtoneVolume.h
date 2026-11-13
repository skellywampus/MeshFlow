#pragma once
#include <cstdint>

constexpr int m9RingtoneDuty(uint32_t frequency, bool playing, uint8_t volume)
{
    if (!frequency || !playing || volume > 3) return -1;
    constexpr int duties[] = {0, 32, 128, 511};
    return duties[volume];
}
