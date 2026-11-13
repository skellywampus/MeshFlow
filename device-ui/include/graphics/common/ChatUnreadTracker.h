#pragma once

#include <cstdint>
#include <unordered_map>

// Keeps the dashboard unread total derived from the per-conversation counts.
class ChatUnreadTracker
{
  public:
    void record(uint32_t conversationId)
    {
        ++total_;
        ++byConversation_[conversationId];
    }

    bool clear(uint32_t conversationId)
    {
        const auto found = byConversation_.find(conversationId);
        if (found == byConversation_.end())
            return false;

        total_ -= found->second;
        byConversation_.erase(found);
        return true;
    }

    void clearAll()
    {
        byConversation_.clear();
        total_ = 0;
    }

    uint32_t total() const { return total_; }

    uint32_t countFor(uint32_t conversationId) const
    {
        const auto found = byConversation_.find(conversationId);
        return found == byConversation_.end() ? 0 : found->second;
    }

  private:
    std::unordered_map<uint32_t, uint32_t> byConversation_;
    uint32_t total_ = 0;
};
