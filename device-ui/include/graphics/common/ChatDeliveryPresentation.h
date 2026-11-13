#pragma once

#include "util/LogMessage.h"

#include <string>
#include <string_view>

inline const char *chatDeliveryStatusLabel(LogMessage::MsgStatus status)
{
    switch (status) {
    case LogMessage::eHeard:
        return "Heard";
    case LogMessage::eAcked:
        return "Acknowledged";
    case LogMessage::eFailed:
        return "Failed";
    case LogMessage::eNoResponse:
        return "No response";
    default:
        return nullptr;
    }
}

inline std::string formatChatBubbleText(std::string_view message, const char *deliveryStatus)
{
    std::string text(message);
    if (deliveryStatus) {
        text.push_back('\n');
        text.append(deliveryStatus);
    }
    return text;
}
