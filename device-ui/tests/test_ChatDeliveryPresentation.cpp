#include "graphics/common/ChatDeliveryPresentation.h"

#include <doctest/doctest.h>

TEST_CASE("chat delivery states have readable labels")
{
    CHECK(chatDeliveryStatusLabel(LogMessage::eNone) == nullptr);
    CHECK(std::string(chatDeliveryStatusLabel(LogMessage::eHeard)) == "Heard");
    CHECK(std::string(chatDeliveryStatusLabel(LogMessage::eAcked)) == "Acknowledged");
    CHECK(std::string(chatDeliveryStatusLabel(LogMessage::eFailed)) == "Failed");
    CHECK(std::string(chatDeliveryStatusLabel(LogMessage::eNoResponse)) == "No response");
}

TEST_CASE("chat bubble adds a readable delivery state below the message")
{
    CHECK(formatChatBubbleText("20:42\nMeet at the trailhead", "Acknowledged") ==
          "20:42\nMeet at the trailhead\nAcknowledged");
    CHECK(formatChatBubbleText("20:42\nStill sending", nullptr) == "20:42\nStill sending");
}
