#pragma once

namespace GpsModeTransition
{
template <typename Mode>
constexpr Mode toggle(Mode current, Mode notPresent, Mode disabled, Mode enabled)
{
    return current == notPresent || current == disabled ? enabled : disabled;
}
} // namespace GpsModeTransition
