#include "graphics/common/GpsModeTransition.h"

#include <doctest/doctest.h>

TEST_CASE("GPS mode can be enabled when disabled or not present")
{
    constexpr int notPresent = 0;
    constexpr int disabled = 1;
    constexpr int enabled = 2;

    CHECK(GpsModeTransition::toggle(notPresent, notPresent, disabled, enabled) == enabled);
    CHECK(GpsModeTransition::toggle(disabled, notPresent, disabled, enabled) == enabled);
}

TEST_CASE("GPS mode can be disabled when enabled")
{
    constexpr int notPresent = 0;
    constexpr int disabled = 1;
    constexpr int enabled = 2;

    CHECK(GpsModeTransition::toggle(enabled, notPresent, disabled, enabled) == disabled);
}
