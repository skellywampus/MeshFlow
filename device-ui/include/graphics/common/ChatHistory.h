#pragma once

#include "graphics/common/NodeStore.h"
#include "util/LogMessage.h"
#include <algorithm>
#include <string>
#include <vector>

class ChatHistory
{
  public:
    struct Entry {
        std::basic_string<char, std::char_traits<char>, NodeStoreAllocator<char>> text;
        uint32_t node = 0;
        uint64_t sequence = 0;
        uint32_t requestId = 0;
        uint32_t to = UINT32_MAX;
        uint8_t channel = 0;
        std::basic_string<char, std::char_traits<char>, NodeStoreAllocator<char>> payload;
        LogMessage::MsgStatus status = LogMessage::eNone;
        bool sent = false;
    };
    static constexpr size_t window = 100;
    static constexpr size_t step = 25;
    static constexpr size_t retainedEntries = 1024;

    size_t append(Entry entry, bool follow)
    {
        const size_t visibleCount = end >= begin ? end - begin : 0;
        entries.push_back(std::move(entry));
        const size_t trimmed = entries.size() > retainedEntries ? entries.size() - retainedEntries : 0;
        if (trimmed) {
            entries.erase(entries.begin(), entries.begin() + trimmed);
            begin = begin > trimmed ? begin - trimmed : 0;
            end = std::min(entries.size(), begin + visibleCount);
        }
        if (follow)
            latest();
        return trimmed;
    }
    bool dropOldest()
    {
        if (entries.empty())
            return false;
        const size_t visibleCount = end >= begin ? end - begin : 0;
        entries.erase(entries.begin());
        begin = begin ? begin - 1 : 0;
        end = std::min(entries.size(), begin + visibleCount);
        return true;
    }
    void latest()
    {
        end = entries.size();
        begin = end > window ? end - window : 0;
    }
    bool older()
    {
        if (!begin)
            return false;
        begin -= std::min(begin, step);
        end = std::min(entries.size(), begin + window);
        return true;
    }
    bool newer()
    {
        if (end == entries.size())
            return false;
        end += std::min(entries.size() - end, step);
        begin = end > window ? end - window : 0;
        return true;
    }
    void setStatus(uint32_t requestId, LogMessage::MsgStatus status)
    {
        if (!requestId)
            return;
        for (auto position = entries.rbegin(); position != entries.rend(); ++position) {
            if (position->sent && position->requestId == requestId) {
                if (status != LogMessage::eNoResponse || position->status == LogMessage::eNone)
                    position->status = status;
                return;
            }
        }
    }
    const Entry &at(size_t index) const { return entries.at(index); }
    Entry *findRequest(uint32_t requestId)
    {
        for (auto it = entries.rbegin(); it != entries.rend(); ++it)
            if (it->sent && it->requestId == requestId)
                return &*it;
        return nullptr;
    }
    size_t size() const { return entries.size(); }
    size_t first() const { return begin; }
    size_t last() const { return end; }
    bool atLatest() const { return end == entries.size(); }
    int32_t scrollY = 0;
    bool opened = false;

  private:
    std::vector<Entry, NodeStoreAllocator<Entry>> entries;
    size_t begin = 0;
    size_t end = 0;
};
