#include "graphics/common/ChatHistory.h"
#include "graphics/common/ChatHistoryBudget.h"
#include <doctest/doctest.h>
#include <map>

namespace
{
ChatHistory historyWithMessages(size_t count)
{
    ChatHistory history;
    for (size_t index = 0; index < count; ++index) {
        ChatHistory::Entry entry;
        entry.text = std::to_string(index).c_str();
        history.append(std::move(entry), true);
    }
    return history;
}
} // namespace

TEST_CASE("chat pages stay bounded while traversing the entire history in both directions")
{
    auto history = historyWithMessages(1000);
    CHECK(history.first() == 900);
    CHECK(history.last() == 1000);
    while (history.older())
        CHECK(history.last() - history.first() <= ChatHistory::window);
    CHECK(history.first() == 0);
    CHECK(history.at(0).text == "0");
    while (history.newer())
        CHECK(history.last() - history.first() <= ChatHistory::window);
    CHECK(history.last() == 1000);
    CHECK(history.size() == 1000);
}

TEST_CASE("incoming messages preserve a historical page and its reading bookmark")
{
    auto history = historyWithMessages(250);
    REQUIRE(history.older());
    const size_t first = history.first();
    const size_t last = history.last();
    history.scrollY = 37;
    history.opened = true;
    history.append(ChatHistory::Entry{}, false);
    CHECK(history.first() == first);
    CHECK(history.last() == last);
    CHECK(history.scrollY == 37);
    CHECK(history.opened);
    while (history.newer()) {
    }
    CHECK(history.last() == 251);
    CHECK(history.last() - history.first() == ChatHistory::window);
}

TEST_CASE("chat delivery status survives paging and timeout does not downgrade a response")
{
    ChatHistory history;
    ChatHistory::Entry entry;
    entry.sent = true;
    entry.requestId = 42;
    history.append(std::move(entry), true);
    for (size_t index = 0; index < 250; ++index)
        history.append(ChatHistory::Entry{}, true);
    history.setStatus(42, LogMessage::eHeard);
    history.setStatus(42, LogMessage::eNoResponse);
    CHECK(history.at(0).status == LogMessage::eHeard);
    history.setStatus(42, LogMessage::eAcked);
    while (history.older()) {
    }
    CHECK(history.at(0).status == LogMessage::eAcked);
    history.setStatus(0, LogMessage::eFailed);
    CHECK(history.at(1).status == LogMessage::eNone);
}

TEST_CASE("small chat pages and partial page boundaries retain every message")
{
    auto history = historyWithMessages(103);
    CHECK(history.first() == 3);
    REQUIRE(history.older());
    CHECK(history.first() == 0);
    CHECK(history.last() == 100);
    REQUIRE(history.newer());
    CHECK(history.first() == 3);
    CHECK(history.last() == 103);
    auto small = historyWithMessages(3);
    CHECK_FALSE(small.older());
    CHECK_FALSE(small.newer());
    CHECK(small.last() == 3);
}

TEST_CASE("chat history retention is bounded while preserving the newest entries")
{
    auto history = historyWithMessages(ChatHistory::retainedEntries + 476);

    CHECK(history.size() == ChatHistory::retainedEntries);
    CHECK(history.at(0).text == "476");
    CHECK(history.at(ChatHistory::retainedEntries - 1).text == "1499");
    CHECK(history.first() == ChatHistory::retainedEntries - ChatHistory::window);
    CHECK(history.last() == ChatHistory::retainedEntries);
}

TEST_CASE("trimming old messages preserves the visible historical page")
{
    auto history = historyWithMessages(ChatHistory::retainedEntries);
    REQUIRE(history.older());
    const auto firstVisibleText = history.at(history.first()).text;
    const size_t oldFirst = history.first();
    const size_t oldPageSize = history.last() - history.first();

    CHECK(history.append(ChatHistory::Entry{}, false) == 1);

    CHECK(history.size() == ChatHistory::retainedEntries);
    CHECK(history.first() + 1 == oldFirst);
    CHECK(history.last() - history.first() == oldPageSize);
    CHECK(history.at(history.first()).text == firstVisibleText);
    CHECK(history.at(history.last() - 1).text == "998");
}

TEST_CASE("dropping an entry preserves the page content when it rolls off the front")
{
    auto history = historyWithMessages(300);
    REQUIRE(history.older());
    const size_t oldFirst = history.first();
    const size_t oldLast = history.last();
    const auto firstVisible = history.at(oldFirst).text;

    CHECK(history.dropOldest());

    CHECK(history.first() + 1 == oldFirst);
    CHECK(history.last() + 1 == oldLast);
    CHECK(history.at(history.first()).text == firstVisible);
    CHECK(history.dropOldest());
}

TEST_CASE("global chat budget evicts the oldest entry across conversations")
{
    std::map<int, ChatHistory> histories;
    auto add = [&](int conversation, uint64_t sequence) {
        ChatHistory::Entry entry;
        entry.sequence = sequence;
        entry.text = std::to_string(sequence).c_str();
        histories[conversation].append(std::move(entry), true);
    };
    add(1, 30);
    add(1, 40);
    add(2, 10);
    add(3, 20);
    size_t entryCount = 4;
    int trimmedConversation = -1;

    CHECK(ChatHistoryBudget::trim(histories, entryCount,
                                  [&](int conversation, ChatHistory &) { trimmedConversation = conversation; }, 3) == 1);

    CHECK(entryCount == 3);
    CHECK(trimmedConversation == 2);
    CHECK(histories.at(2).size() == 0);
    CHECK(histories.at(1).at(0).sequence == 30);
}
