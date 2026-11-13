#if defined(ELECROW_ThinkNode_M9) && HAS_TFT
#include "graphics/view/TFT/TFTView_320x240.h"
#include "graphics/common/M9PowerStatistics.h"
#include "ui.h"
#include "styles.h"
#include "fonts.h"
#include "input/policy/InputContextState.h"

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
        lv_obj_set_width(self->nodeContextLabel, lv_pct(100));
        lv_label_set_long_mode(self->nodeContextLabel, LV_LABEL_LONG_WRAP);
        // Keep Up/Down as scrolling keys instead of cycling the sole Back control.
        input_policy::InputContextState::instance().setFocusSemantic(input_policy::FocusSemantic::Scrollable);
        // Match current system panel/button styling; Up/Down scroll the content,
        // Center or Escape on Back closes the modal through the existing handler.
        lv_obj_set_style_text_font(self->nodeContextLabel, &ui_font_montserrat_12, 0);
    }, LV_EVENT_CLICKED, nullptr);
}

void TFTView_320x240::refreshPowerStatistics()
{
    if (!powerStatisticsView || !nodeContextBox || !nodeContextLabel) return;
    char text[4096];
    meshflow_power_text(text, sizeof(text));
    if (strcmp(lv_label_get_text(nodeContextLabel), text) != 0)
        lv_label_set_text(nodeContextLabel, text);
}
#endif
