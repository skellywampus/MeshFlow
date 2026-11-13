#pragma once

#include "lvgl.h"
#include <algorithm>
#include <cstdint>
#include <vector>

struct ChatConversationRecency {
    lv_obj_t *row;
    uint64_t sequence;
    uint32_t previousIndex;
};

inline void moveConversationToFront(lv_obj_t *conversationPanel, lv_obj_t *templateRow, lv_obj_t *conversationRow)
{
    if (!conversationPanel || !conversationRow || lv_obj_get_parent(conversationRow) != conversationPanel) {
        return;
    }

    const bool hasTemplate = templateRow && lv_obj_get_parent(templateRow) == conversationPanel;
    lv_obj_move_to_index(conversationRow, hasTemplate ? 1 : 0);
}

inline void orderConversationsByRecency(lv_obj_t *conversationPanel, lv_obj_t *templateRow,
                                        std::vector<ChatConversationRecency> &rows)
{
    if (!conversationPanel)
        return;

    std::sort(rows.begin(), rows.end(), [](const ChatConversationRecency &left, const ChatConversationRecency &right) {
        if (left.sequence != right.sequence)
            return left.sequence > right.sequence;
        return left.previousIndex < right.previousIndex;
    });

    const bool hasTemplate = templateRow && lv_obj_get_parent(templateRow) == conversationPanel;
    uint32_t index = hasTemplate ? 1 : 0;
    for (const auto &row : rows) {
        if (row.row && lv_obj_get_parent(row.row) == conversationPanel) {
            if (lv_obj_get_index(row.row) != index)
                lv_obj_move_to_index(row.row, index);
            ++index;
        }
    }
}
