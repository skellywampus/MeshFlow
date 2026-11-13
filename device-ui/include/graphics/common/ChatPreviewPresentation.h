#pragma once

#include <cctype>
#include <string>
#include <string_view>

struct ChatPreviewFields {
    std::string text;
    std::string time;
};

inline ChatPreviewFields formatChatPreview(std::string_view formattedMessage, bool includeSender)
{
    const size_t lineBreak = formattedMessage.find('\n');
    if (lineBreak == std::string_view::npos)
        return {std::string(formattedMessage), {}};

    const std::string_view header = formattedMessage.substr(0, lineBreak);
    const size_t colon = header.rfind(':');
    if (colon == std::string_view::npos || colon < 2 || colon + 2 >= header.size())
        return {std::string(formattedMessage), {}};

    const size_t timeStart = colon - 2;
    const auto digit = [](char value) { return std::isdigit(static_cast<unsigned char>(value)) != 0; };
    if (!digit(header[timeStart]) || !digit(header[timeStart + 1]) || !digit(header[colon + 1]) ||
        !digit(header[colon + 2]))
        return {std::string(formattedMessage), {}};

    const unsigned hour = static_cast<unsigned>(header[timeStart] - '0') * 10U +
                          static_cast<unsigned>(header[timeStart + 1] - '0');
    const unsigned minute = static_cast<unsigned>(header[colon + 1] - '0') * 10U +
                            static_cast<unsigned>(header[colon + 2] - '0');
    if (hour > 23 || minute > 59)
        return {std::string(formattedMessage), {}};

    ChatPreviewFields fields;
    fields.time.assign(header.substr(timeStart, 5));
    fields.text.assign(formattedMessage.substr(lineBreak + 1));

    if (includeSender) {
        const size_t senderEnd = header.find(' ');
        const std::string_view sender = header.substr(0, senderEnd);
        if (!sender.empty())
            fields.text.insert(0, std::string(sender) + ": ");
    }
    return fields;
}
