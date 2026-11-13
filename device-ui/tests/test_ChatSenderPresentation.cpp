#include "graphics/common/ChatSenderPresentation.h"

#include <doctest/doctest.h>
#include <string>

TEST_CASE("only incoming channel messages get a sender line")
{
    CHECK(ChatSenderPresentation::isIncomingGroupMessage(1, 8, false, 0x12345678));
    CHECK_FALSE(ChatSenderPresentation::isIncomingGroupMessage(1, 8, true, 0x12345678));
    CHECK_FALSE(ChatSenderPresentation::isIncomingGroupMessage(0x12345678, 8, false, 0x87654321));
    CHECK_FALSE(ChatSenderPresentation::isIncomingGroupMessage(1, 8, false, 0));
}

TEST_CASE("group sender line prefers short name and falls back to long name")
{
    CHECK(std::string(ChatSenderPresentation::preferredName("ABCD", "River Runner")) == "ABCD");
    CHECK(std::string(ChatSenderPresentation::preferredName("", "River Runner")) == "River Runner");
    CHECK(ChatSenderPresentation::preferredName(nullptr, "") == nullptr);
}
