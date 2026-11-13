#pragma once

// Limit fast-memory placement to the M9 software-rendering and text hot paths.
#define LV_ATTRIBUTE_FAST_MEM __attribute__((section(".iram1.lvgl")))
