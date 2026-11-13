#pragma once

#include <cstdint>

// Translate the ThinkNode M9 controller's hardware key bytes into the
// Meshtastic MUI key space. Values for LVGL keys are supplied by the caller so
// this mapping can also be exercised by host-side tests without LVGL headers.
class M9KeyboardKeyMapper
{
  public:
    static constexpr bool SUPPORTS_LONG_PRESS = true;

    struct Result {
        uint32_t key;
        bool syntheticLongPress;
        bool ignored;
    };

    static constexpr Result map(uint32_t rawKey, uint32_t enterKey, uint32_t escapeKey, uint32_t backspaceKey,
                                uint32_t leftKey, uint32_t upKey, uint32_t downKey, uint32_t rightKey)
    {
        switch (rawKey) {
        case 0x0D: // Center / Enter
            return {enterKey, false, false};
        case 0x81: // Messages
            return {0x100, false, false};
        case 0x82: // Home / launcher
            return {0x101, false, false};
        case 0x83: // Quick chat
            return {0x102, false, false};
        case 0x84: // Send location ping
            return {0x103, false, false};
        case 0x85: // Map
            return {0x104, false, false};
        case 0x86: // Back
            return {escapeKey, false, false};
        case 0x87: // Long-press location / GPS toggle
            return {0x105, false, false};
        case 0x90: // Ctrl / quick Control Center access
            return {0x106, false, false};
        case 0x88: // Triangle / focused-item hold action
        case 0xA3: // Controller-reported long Enter
            return {enterKey, true, false};
        case 0x89: // Unidentified controller key: deliberately unbound.
            return {0, false, true};
        case 0x08: // Delete
            return {backspaceKey, false, false};
        case 0xB4:
            return {leftKey, false, false};
        case 0xB5:
            return {upKey, false, false};
        case 0xB6:
            return {downKey, false, false};
        case 0xB7:
            return {rightKey, false, false};
        default:
            return {rawKey, false, false};
        }
    }
};
