#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace ChatUnreadBadgePresentation {

inline bool isVisible(uint32_t unreadCount) { return unreadCount > 0; }

class VisibilityState
{
  public:
    bool update(uint32_t unreadCount)
    {
        const bool nextVisible = isVisible(unreadCount);
        const bool changed = !initialized_ || nextVisible != visible_;
        initialized_ = true;
        visible_ = nextVisible;
        return changed;
    }

    bool visible() const { return visible_; }

  private:
    bool initialized_ = false;
    bool visible_ = false;
};

inline void format(char *buffer, size_t bufferSize, uint32_t unreadCount)
{
    if (!buffer || bufferSize == 0)
        return;
    if (unreadCount > 99)
        std::snprintf(buffer, bufferSize, "99+");
    else
        std::snprintf(buffer, bufferSize, "%u", static_cast<unsigned>(unreadCount));
}

} // namespace ChatUnreadBadgePresentation
