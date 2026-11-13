#pragma once

#include "graphics/view/TFT/Themes.h"
#include "lvgl.h"

// Apply the opposite theme to the focused surface. USER_1 is local to the
// focused surface and labels, so existing per-item colors return on defocus.
namespace M9FocusTheme
{
inline lv_obj_tree_walk_res_t colorLabel(lv_obj_t *obj, void *data)
{
    const bool focused = data != nullptr;
    // Unread badges keep their own high-contrast colors when their card is focused.
    if (lv_obj_has_flag(obj, LV_OBJ_FLAG_USER_1))
        return LV_OBJ_TREE_WALK_NEXT;
    if (lv_obj_check_type(obj, &lv_label_class)) {
        if (focused) {
            const bool dark = Themes::get() == Themes::eDark;
            lv_obj_set_style_text_color(obj, lv_color_hex(dark ? 0x203b2c : 0xe7f3ea), LV_STATE_USER_1);
            lv_obj_add_state(obj, LV_STATE_USER_1);
        } else {
            lv_obj_remove_state(obj, LV_STATE_USER_1);
        }
    } else if (lv_obj_check_type(obj, &lv_image_class)) {
        if (focused) {
            const bool dark = Themes::get() == Themes::eDark;
            const lv_color_t color = lv_color_hex(dark ? 0x203b2c : 0xe7f3ea);
            lv_obj_set_style_image_recolor(obj, color, LV_STATE_USER_1);
            lv_obj_set_style_image_recolor_opa(obj, LV_OPA_COVER, LV_STATE_USER_1);
            lv_obj_set_style_bg_image_recolor(obj, color, LV_STATE_USER_1);
            lv_obj_set_style_bg_image_recolor_opa(obj, LV_OPA_COVER, LV_STATE_USER_1);
            lv_obj_add_state(obj, LV_STATE_USER_1);
        } else {
            lv_obj_remove_state(obj, LV_STATE_USER_1);
        }
    }
    return LV_OBJ_TREE_WALK_NEXT;
}

inline void event(lv_event_t *event)
{
    const auto code = lv_event_get_code(event);
    if (code != LV_EVENT_FOCUSED && code != LV_EVENT_DEFOCUSED)
        return;
    auto *focusedObj = lv_event_get_target_obj(event);
    if (focusedObj != lv_event_get_current_target_obj(event))
        return;
    auto *surface = static_cast<lv_obj_t *>(lv_event_get_user_data(event));
    if (!surface)
        surface = focusedObj;
    if (code == LV_EVENT_FOCUSED) {
        const bool dark = Themes::get() == Themes::eDark;
        lv_obj_set_style_bg_color(surface, lv_color_hex(dark ? 0xf3f8f4 : 0x18271d), LV_STATE_USER_1);
        lv_obj_set_style_bg_opa(surface, LV_OPA_COVER, LV_STATE_USER_1);
        lv_obj_set_style_text_color(surface, lv_color_hex(dark ? 0x203b2c : 0xe7f3ea), LV_STATE_USER_1);
        lv_obj_set_style_bg_image_recolor(surface, lv_color_hex(dark ? 0x203b2c : 0xe7f3ea), LV_STATE_USER_1);
        lv_obj_set_style_bg_image_recolor_opa(surface, LV_OPA_COVER, LV_STATE_USER_1);
        lv_obj_set_style_outline_width(surface, 0, LV_STATE_USER_1);
        lv_obj_set_style_border_width(surface, 0, LV_STATE_USER_1);
        lv_obj_add_state(surface, LV_STATE_USER_1);
        // The node row's focusable transparent button is a child of its surface.
        lv_obj_set_style_outline_width(focusedObj, 0, LV_STATE_FOCUS_KEY);
        lv_obj_tree_walk(surface, colorLabel, surface);
    } else {
        lv_obj_tree_walk(surface, colorLabel, nullptr);
        lv_obj_remove_state(surface, LV_STATE_USER_1);
    }
}

inline void attach(lv_obj_t *obj, lv_obj_t *surface = nullptr)
{
    if (obj)
        lv_obj_add_event_cb(obj, event, LV_EVENT_ALL, surface);
}
} // namespace M9FocusTheme
