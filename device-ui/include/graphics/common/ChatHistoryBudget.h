#pragma once

#include <cstddef>

class ChatHistoryBudget
{
  public:
    static constexpr size_t maxEntries = 2048;

    template <typename HistoryMap, typename OnTrim>
    static size_t trim(HistoryMap &histories, size_t &entryCount, OnTrim &&onTrim, size_t limit = maxEntries)
    {
        size_t trimmed = 0;
        while (entryCount > limit) {
            auto oldest = histories.end();
            for (auto current = histories.begin(); current != histories.end(); ++current) {
                if (!current->second.size())
                    continue;
                if (oldest == histories.end() || current->second.at(0).sequence < oldest->second.at(0).sequence)
                    oldest = current;
            }
            if (oldest == histories.end())
                break;

            onTrim(oldest->first, oldest->second);
            if (!oldest->second.dropOldest())
                break;
            --entryCount;
            ++trimmed;
        }
        return trimmed;
    }
};
