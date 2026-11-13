#include "graphics/common/ChatUnreadTracker.h"
#include "graphics/common/ChatUnreadBadgePresentation.h"
#include <doctest/doctest.h>
#include <string>

TEST_CASE("clearing one conversation preserves other unread counts")
{
    ChatUnreadTracker unread;
    unread.record(0); // Channel zero is a valid conversation key.
    unread.record(0);
    unread.record(42);

    CHECK(unread.total() == 3);
    CHECK(unread.countFor(0) == 2);
    CHECK(unread.countFor(42) == 1);

    CHECK(unread.clear(0));
    CHECK(unread.total() == 1);
    CHECK(unread.countFor(0) == 0);
    CHECK(unread.countFor(42) == 1);
}

TEST_CASE("clearing an unknown conversation does not change unread state")
{
    ChatUnreadTracker unread;
    unread.record(7);

    CHECK_FALSE(unread.clear(8));
    CHECK(unread.total() == 1);
    CHECK(unread.countFor(7) == 1);
}

TEST_CASE("clear all resets per-conversation and dashboard unread counts together")
{
    ChatUnreadTracker unread;
    unread.record(1);
    unread.record(2);

    unread.clearAll();
    CHECK(unread.total() == 0);
    CHECK(unread.countFor(1) == 0);
    CHECK(unread.countFor(2) == 0);
}

TEST_CASE("launcher unread badge hides zero and caps large counts")
{
    char text[5];
    CHECK_FALSE(ChatUnreadBadgePresentation::isVisible(0));
    CHECK(ChatUnreadBadgePresentation::isVisible(1));

    ChatUnreadBadgePresentation::format(text, sizeof(text), 1);
    CHECK(std::string(text) == "1");
    ChatUnreadBadgePresentation::format(text, sizeof(text), 99);
    CHECK(std::string(text) == "99");
    ChatUnreadBadgePresentation::format(text, sizeof(text), 100);
    CHECK(std::string(text) == "99+");
}

TEST_CASE("unread visibility only changes on zero boundary transitions")
{
    ChatUnreadBadgePresentation::VisibilityState state;

    CHECK(state.update(0)); // Initialize from the current unread count.
    CHECK_FALSE(state.visible());
    CHECK_FALSE(state.update(0));
    CHECK(state.update(1));
    CHECK(state.visible());
    CHECK_FALSE(state.update(2));
    CHECK_FALSE(state.update(99));
    CHECK(state.update(0));
    CHECK_FALSE(state.visible());
}
