#if defined(ELECROW_ThinkNode_M9) && HAS_TFT
#include "graphics/view/TFT/TFTView_320x240.h"
#include "graphics/common/M9PowerStatistics.h"
#include "ui.h"
#include "styles.h"
#include "fonts.h"
#include "input/policy/InputContextState.h"
#include <string>

namespace {
constexpr unsigned maxSections = 8;
lv_obj_t *sectionCards[maxSections]{};
lv_obj_t *sectionTitles[maxSections]{};
lv_obj_t *sectionBodies[maxSections]{};

void renderPowerSections(lv_obj_t *content, const char *text, bool create)
{
    std::string all(text);
    size_t start = 0;
    for (unsigned i = 0; i < maxSections; ++i) {
        while (start < all.size() && all[start] == '\n') ++start;
        size_t end = all.find("\n\n", start);
        if (end == std::string::npos) end = all.size();
        if (start >= all.size()) {
            if (!create && sectionCards[i]) lv_obj_add_flag(sectionCards[i], LV_OBJ_FLAG_HIDDEN);
            continue;
        }
        const size_t lineEnd = all.find('\n', start);
        const size_t titleEnd = lineEnd == std::string::npos || lineEnd > end ? end : lineEnd;
        std::string title = all.substr(start, titleEnd - start);
        std::string body = titleEnd < end ? all.substr(titleEnd + 1, end - titleEnd - 1) : "";
        if (create) {
            auto *card = sectionCards[i] = lv_obj_create(content);
            add_style_settings_panel_style(card);
            lv_obj_set_size(card, lv_pct(100), LV_SIZE_CONTENT);
            lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
            lv_obj_set_style_radius(card, 6, 0);
            lv_obj_set_style_border_width(card, 1, 0);
            lv_obj_set_style_pad_all(card, 6, 0);
            lv_obj_set_style_pad_row(card, 4, 0);
            sectionTitles[i] = lv_label_create(card);
            sectionBodies[i] = lv_label_create(card);
            for (auto *label : {sectionTitles[i], sectionBodies[i]}) {
                lv_obj_set_width(label, lv_pct(100));
                lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
                lv_obj_set_style_text_font(label, &ui_font_montserrat_12, 0);
            }
            lv_obj_set_style_text_line_space(sectionBodies[i], 3, 0);
        }
        if (sectionCards[i]) {
            lv_obj_remove_flag(sectionCards[i], LV_OBJ_FLAG_HIDDEN);
            if (strcmp(lv_label_get_text(sectionTitles[i]), title.c_str()))
                lv_label_set_text(sectionTitles[i], title.c_str());
            if (strcmp(lv_label_get_text(sectionBodies[i]), body.c_str()))
                lv_label_set_text(sectionBodies[i], body.c_str());
        }
        start = end;
    }
}
}

void TFTView_320x240::setupPowerStatistics()
{
    auto *parent = lv_obj_get_parent(objects.tools_statistics_button);
    powerStatisticsButton = lv_button_create(parent);
    lv_obj_set_size(powerStatisticsButton, lv_pct(95), 30);
    lv_obj_add_flag(powerStatisticsButton, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_remove_flag(powerStatisticsButton, LV_OBJ_FLAG_SCROLL_WITH_ARROW);
    add_style_settings_button_style(powerStatisticsButton);
    auto *label = lv_label_create(powerStatisticsButton);
    lv_label_set_text(label, "Power Statistics");
    lv_obj_center(label);
    lv_group_add_obj(defaultPanelGroup, powerStatisticsButton);
    lv_obj_add_event_cb(powerStatisticsButton, [](lv_event_t *) {
        auto *self = currentInstance();
        if (!self) return;
        self->nodeContextReturn = 0;
        char text[4096];
        meshflow_power_text(text, sizeof(text));
        self->nodeContextPopup("Power Statistics", text, false);
        self->powerStatisticsView = true;
        // Use the M9 app canvas instead of the narrow shared dialog geometry.
        // The existing content viewport and Back key handler retain scrolling.
        lv_obj_set_size(self->nodeContextBox, 316, 238);
        lv_obj_set_style_border_width(self->nodeContextBox, 0, LV_PART_MAIN);
        auto *content = lv_msgbox_get_content(self->nodeContextBox);
        lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLL_ELASTIC);
        lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLL_MOMENTUM);
        lv_obj_set_scroll_dir(content, LV_DIR_VER);
        lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_AUTO);
        lv_obj_set_style_pad_hor(content, 8, 0);
        lv_obj_set_style_pad_ver(content, 4, 0);
        lv_obj_set_style_pad_row(content, 8, 0);
        lv_obj_add_flag(self->nodeContextLabel, LV_OBJ_FLAG_HIDDEN);
        for (unsigned i = 0; i < maxSections; ++i) sectionCards[i] = sectionTitles[i] = sectionBodies[i] = nullptr;
        renderPowerSections(content, text, true);
        // Keep Up/Down as scrolling keys instead of cycling the sole Back control.
        input_policy::InputContextState::instance().setFocusSemantic(input_policy::FocusSemantic::Scrollable);
        // Match current system panel/button styling; Up/Down scroll the content,
        // Center or Escape on Back closes the modal through the existing handler.
    }, LV_EVENT_CLICKED, nullptr);
}

void TFTView_320x240::refreshPowerStatistics()
{
    if (!powerStatisticsView || !nodeContextBox || !nodeContextLabel) return;
    char text[4096];
    meshflow_power_text(text, sizeof(text));
    if (strcmp(lv_label_get_text(nodeContextLabel), text) != 0) {
        lv_label_set_text(nodeContextLabel, text);
        renderPowerSections(lv_msgbox_get_content(nodeContextBox), text, false);
    }
}
#endif
