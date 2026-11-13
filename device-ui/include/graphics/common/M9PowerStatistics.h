#pragma once
#include <cstddef>
#include <cstdint>
extern "C" void meshflow_power_state(uint32_t mhz, bool screenOn);
extern "C" void meshflow_power_sample();
extern "C" void meshflow_power_text(char *buffer, size_t capacity);
