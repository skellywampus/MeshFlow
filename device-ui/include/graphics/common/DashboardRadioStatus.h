#pragma once

#include "meshtastic/mesh.pb.h"

#include <cstddef>

namespace DashboardRadioStatus
{
inline const meshtastic_Channel *findPrimaryChannel(const meshtastic_Channel *channels, size_t channelCount)
{
    if (!channels)
        return nullptr;

    for (size_t i = 0; i < channelCount; ++i) {
        if (channels[i].has_settings && channels[i].role == meshtastic_Channel_Role_PRIMARY)
            return &channels[i];
    }
    return nullptr;
}
} // namespace DashboardRadioStatus
