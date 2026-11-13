#include "graphics/common/DashboardRadioStatus.h"

#include <doctest/doctest.h>

TEST_CASE("dashboard radio status finds the configured primary channel outside slot zero")
{
    meshtastic_Channel channels[3]{};
    channels[0].has_settings = true;
    channels[0].role = meshtastic_Channel_Role_SECONDARY;
    channels[1].has_settings = true;
    channels[1].role = meshtastic_Channel_Role_PRIMARY;

    CHECK(DashboardRadioStatus::findPrimaryChannel(channels, 3) == &channels[1]);
}

TEST_CASE("dashboard radio status ignores unset primary roles")
{
    meshtastic_Channel channels[2]{};
    channels[0].role = meshtastic_Channel_Role_PRIMARY;

    CHECK(DashboardRadioStatus::findPrimaryChannel(channels, 2) == nullptr);
    CHECK(DashboardRadioStatus::findPrimaryChannel(nullptr, 2) == nullptr);
}
