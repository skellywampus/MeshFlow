#pragma once

#include "input/policy/InputTypes.h"
#include <cstddef>

// Geometric navigation for the M9 launcher's fixed two-column, four-row grid.
class LauncherGridNavigation
{
  public:
    static constexpr size_t COLUMN_COUNT = 2;
    static constexpr size_t ROW_COUNT = 4;
    static constexpr size_t TILE_COUNT = COLUMN_COUNT * ROW_COUNT;

    static constexpr size_t nextIndex(size_t current, input_policy::InputAction direction)
    {
        if (current >= TILE_COUNT)
            current = 0;

        switch (direction) {
        case input_policy::InputAction::NavigateUp:
            return current < COLUMN_COUNT ? current + (ROW_COUNT - 1) * COLUMN_COUNT : current - COLUMN_COUNT;
        case input_policy::InputAction::NavigateDown:
            return current + COLUMN_COUNT >= TILE_COUNT ? current - (ROW_COUNT - 1) * COLUMN_COUNT
                                                        : current + COLUMN_COUNT;
        case input_policy::InputAction::NavigateLeft:
        case input_policy::InputAction::NavigateRight:
            return current % COLUMN_COUNT == 0 ? current + 1 : current - 1;
        default:
            return current;
        }
    }
};
