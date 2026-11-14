#pragma once

#include <cstdint>

namespace ChatSenderPresentation {

inline bool isIncomingGroupMessage(uint32_t conversationId, uint32_t channelCount, bool sent, uint32_t senderId)
{
    return conversationId < channelCount && !sent && senderId != 0;
}

inline const char *preferredName(const char *longName, const char *shortName)
{
    if (longName && longName[0])
        return longName;
    if (shortName && shortName[0])
        return shortName;
    return nullptr;
}

} // namespace ChatSenderPresentation
