#include "graphics/common/ChatPreviewPresentation.h"

#include <doctest/doctest.h>

TEST_CASE("direct chat preview shows the message body and timestamp")
{
    const auto fields = formatChatPreview("20:42\nMeet at the trailhead", false);

    CHECK(fields.text == "Meet at the trailhead");
    CHECK(fields.time == "20:42");
}

TEST_CASE("group chat preview retains its sender prefix and timestamp")
{
    const auto fields = formatChatPreview("ABCD 26/11/07 20:42\nOn my way", true);

    CHECK(fields.text == "ABCD: On my way");
    CHECK(fields.time == "20:42");
}

TEST_CASE("chat preview preserves messages without a valid timestamp header")
{
    const std::string message = "A multiline message:\nsecond line";
    const auto fields = formatChatPreview(message, false);

    CHECK(fields.text == message);
    CHECK(fields.time.empty());
}

TEST_CASE("chat preview rejects out-of-range clock values")
{
    const auto fields = formatChatPreview("29:99\nMessage", false);

    CHECK(fields.text == "29:99\nMessage");
    CHECK(fields.time.empty());
}
