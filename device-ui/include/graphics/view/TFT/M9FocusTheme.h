#pragma once

#include "graphics/view/TFT/Themes.h"
#include "lvgl.h"

// Invert dark-mode focus; use black/white/yellow focus in light mode. USER_1 is local to the
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
            lv_obj_set_style_text_color(obj, lv_color_hex(dark ? 0x203b2c : 0xffffff), LV_STATE_USER_1);
            lv_obj_add_state(obj, LV_STATE_USER_1);
        } else {
            lv_obj_remove_state(obj, LV_STATE_USER_1);
        }
    } else if (lv_obj_check_type(obj, &lv_image_class)) {
        if (focused) {
            const bool dark = Themes::get() == Themes::eDark;
            const lv_color_t color = lv_color_hex(dark ? 0x203b2c : 0xffffff);
            if (!dark) {
                lv_obj_set_style_bg_color(obj, lv_color_black(), LV_STATE_USER_1);
                lv_obj_set_style_border_color(obj, lv_color_black(), LV_STATE_USER_1);
            } else {
                lv_obj_remove_local_style_prop(obj, LV_STYLE_BG_COLOR, LV_STATE_USER_1);
                lv_obj_remove_local_style_prop(obj, LV_STYLE_BORDER_COLOR, LV_STATE_USER_1);
            }
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
    const bool light = Themes::get() == Themes::eLight;
    if (code != LV_EVENT_FOCUSED && code != LV_EVENT_DEFOCUSED &&
        !(light && (code == LV_EVENT_PRESSED || code == LV_EVENT_RELEASED || code == LV_EVENT_VALUE_CHANGED)))
        return;
    auto *focusedObj = lv_event_get_target_obj(event);
    if (focusedObj != lv_event_get_current_target_obj(event))
        return;
    auto *surface = static_cast<lv_obj_t *>(lv_event_get_user_data(event));
    if (!surface)
        surface = focusedObj;
    const bool selected = light ? ((code != LV_EVENT_DEFOCUSED && lv_obj_has_state(focusedObj, LV_STATE_FOCUSED)) ||
                                  (code != LV_EVENT_RELEASED && lv_obj_has_state(focusedObj, LV_STATE_PRESSED)) ||
                                  lv_obj_has_state(focusedObj, LV_STATE_CHECKED)) : code == LV_EVENT_FOCUSED;
    if (selected) {
        const bool dark = Themes::get() == Themes::eDark;
        lv_obj_set_style_bg_color(surface, lv_color_hex(dark ? 0xf3f8f4 : 0x000000), LV_STATE_USER_1);
        lv_obj_set_style_bg_opa(surface, LV_OPA_COVER, LV_STATE_USER_1);
        lv_obj_set_style_text_color(surface, lv_color_hex(dark ? 0x203b2c : 0xffffff), LV_STATE_USER_1);
        lv_obj_set_style_bg_image_recolor(surface, lv_color_hex(dark ? 0x203b2c : 0xffffff), LV_STATE_USER_1);
        lv_obj_set_style_bg_image_recolor_opa(surface, LV_OPA_COVER, LV_STATE_USER_1);
        lv_obj_set_style_outline_width(surface, 0, LV_STATE_USER_1);
        lv_obj_set_style_border_width(surface, dark ? 0 : 2, LV_STATE_USER_1);
        if (!dark) {
            lv_obj_set_style_border_color(surface, lv_color_hex(0xffd400), LV_STATE_USER_1);
            lv_obj_set_style_border_side(surface, LV_BORDER_SIDE_FULL, LV_STATE_USER_1);
            lv_obj_set_style_shadow_width(surface, 0, LV_STATE_USER_1);
        } else {
            lv_obj_remove_local_style_prop(surface, LV_STYLE_SHADOW_WIDTH, LV_STATE_USER_1);
        }
        lv_obj_add_state(surface, LV_STATE_USER_1);
        // The node row's focusable transparent button is a child of its surface.
        lv_obj_set_style_outline_width(focusedObj, 0, LV_STATE_FOCUS_KEY);
        lv_obj_tree_walk(surface, colorLabel, surface);
    } else {
        lv_obj_tree_walk(surface, colorLabel, nullptr);
        lv_obj_remove_state(surface, LV_STATE_USER_1);
    }
    if (light) {
        // Local setters cancel any inherited LVGL color/border transition.
        // USER_1 remains inactive after defocus, so original default colors return.
        lv_obj_set_style_bg_color(surface, lv_color_black(), LV_STATE_USER_1);
        lv_obj_set_style_text_color(surface, lv_color_white(), LV_STATE_USER_1);
        lv_obj_set_style_border_color(surface, lv_color_hex(0xffd400), LV_STATE_USER_1);
        lv_obj_set_style_border_width(surface, 2, LV_STATE_USER_1);
    }
}

inline void applyLightControl(lv_obj_t *obj)
{
    static lv_style_t selected, disabled, normal, indicator, placeholder;
    static const lv_style_prop_t noTransitionProps[] = {0};
    static lv_style_transition_dsc_t noTransition;
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        lv_style_init(&normal);
        lv_style_set_bg_color(&normal, lv_color_white());
        lv_style_set_text_color(&normal, lv_color_hex(0x101820));
        lv_style_set_image_recolor(&normal, lv_color_black());
        lv_style_set_image_recolor_opa(&normal, LV_OPA_COVER);
        lv_style_set_bg_image_recolor(&normal, lv_color_black());
        lv_style_set_bg_image_recolor_opa(&normal, LV_OPA_COVER);
        lv_style_set_border_color(&normal, lv_color_hex(0x003b70));
        lv_style_set_border_width(&normal, 1);
        lv_style_set_shadow_width(&normal, 0);
        lv_style_transition_dsc_init(&noTransition, noTransitionProps, nullptr, 0, 0, nullptr);
        lv_style_set_transition(&normal, &noTransition);
        lv_style_set_outline_width(&normal, 0);
        lv_style_init(&selected);
        lv_style_set_transition(&selected, &noTransition);
        lv_style_set_bg_color(&selected, lv_color_black());
        lv_style_set_bg_opa(&selected, LV_OPA_COVER);
        lv_style_set_text_color(&selected, lv_color_white());
        lv_style_set_image_recolor(&selected, lv_color_white());
        lv_style_set_image_recolor_opa(&selected, LV_OPA_COVER);
        lv_style_set_bg_image_recolor(&selected, lv_color_white());
        lv_style_set_bg_image_recolor_opa(&selected, LV_OPA_COVER);
        lv_style_set_border_color(&selected, lv_color_hex(0xffd400));
        lv_style_set_border_width(&selected, 2);
        lv_style_set_border_side(&selected, LV_BORDER_SIDE_FULL);
        lv_style_set_outline_width(&selected, 0);
        lv_style_set_shadow_width(&selected, 0);
        lv_style_init(&indicator);
        lv_style_set_bg_color(&indicator, lv_color_hex(0x003b70));
        lv_style_set_bg_opa(&indicator, LV_OPA_COVER);
        lv_style_init(&placeholder);
        lv_style_set_text_color(&placeholder, lv_color_hex(0x334155));
        lv_style_init(&disabled);
        lv_style_set_opa(&disabled, LV_OPA_COVER);
        lv_style_set_text_color(&disabled, lv_color_hex(0x334155));
        lv_style_set_bg_color(&disabled, lv_color_hex(0xe2e8f0));
    }
    const bool light = Themes::get() == Themes::eLight;
    lv_obj_remove_style(obj, &normal, LV_PART_MAIN);
    if (light) lv_obj_add_style(obj, &normal, LV_PART_MAIN);
    const lv_state_t states[] = {LV_STATE_FOCUSED, LV_STATE_FOCUS_KEY, LV_STATE_PRESSED, LV_STATE_CHECKED};
    for (auto state : states) {
        lv_obj_remove_style(obj, &selected, state);
        if (light) lv_obj_add_style(obj, &selected, state);
    }
    if (lv_obj_check_type(obj, &lv_slider_class) || lv_obj_check_type(obj, &lv_switch_class)) {
        lv_obj_remove_style(obj, &indicator, LV_PART_INDICATOR);
        lv_obj_remove_style(obj, &normal, LV_PART_KNOB);
        if (light) {
            lv_obj_add_style(obj, &indicator, LV_PART_INDICATOR);
            lv_obj_add_style(obj, &normal, LV_PART_KNOB);
        }
    }
    if (lv_obj_check_type(obj, &lv_textarea_class)) {
        lv_obj_remove_style(obj, &placeholder, LV_PART_TEXTAREA_PLACEHOLDER);
        if (light) lv_obj_add_style(obj, &placeholder, LV_PART_TEXTAREA_PLACEHOLDER);
    }
    lv_obj_remove_style(obj, &disabled, LV_STATE_DISABLED);
    if (light) lv_obj_add_style(obj, &disabled, LV_STATE_DISABLED);
}

inline void attach(lv_obj_t *obj, lv_obj_t *surface = nullptr)
{
    if (obj) {
        if (!surface)
            applyLightControl(obj);
        lv_obj_add_event_cb(obj, event, LV_EVENT_ALL, surface);
        if (lv_obj_has_state(obj, LV_STATE_FOCUSED))
            lv_obj_send_event(obj, LV_EVENT_FOCUSED, nullptr);
    }
}
inline void lightEvent(lv_event_t *e)
{
    if (Themes::get() == Themes::eLight) event(e);
}

inline lv_obj_tree_walk_res_t refresh(lv_obj_t *obj, void *)
{
    const bool control = lv_obj_check_type(obj, &lv_button_class) || lv_obj_check_type(obj, &lv_imagebutton_class) ||
                         lv_obj_check_type(obj, &lv_dropdown_class) ||
                         lv_obj_check_type(obj, &lv_textarea_class) || lv_obj_check_type(obj, &lv_slider_class) ||
                         lv_obj_check_type(obj, &lv_switch_class) || lv_obj_check_type(obj, &lv_checkbox_class);
    bool attached = false, lightAttached = false, separateSurface = false;
    for (uint32_t i = 0; i < lv_obj_get_event_count(obj); ++i)
    {
        const auto cb = lv_event_dsc_get_cb(lv_obj_get_event_dsc(obj, i));
        if (cb == event) {
            attached = true;
            separateSurface = lv_event_dsc_get_user_data(lv_obj_get_event_dsc(obj, i)) != nullptr;
        }
        if (cb == lightEvent) lightAttached = true;
    }
    if (control || attached) {
        if (!separateSurface)
            applyLightControl(obj);
        if (!attached && !lightAttached && Themes::get() == Themes::eLight)
            lv_obj_add_event_cb(obj, lightEvent, LV_EVENT_ALL, nullptr);
        if (!attached && Themes::get() != Themes::eLight) {
            lv_obj_tree_walk(obj, colorLabel, nullptr);
            lv_obj_remove_state(obj, LV_STATE_USER_1);
        }
        if (attached || Themes::get() == Themes::eLight)
            lv_obj_send_event(obj, lv_obj_has_state(obj, LV_STATE_FOCUSED) ? LV_EVENT_FOCUSED : LV_EVENT_DEFOCUSED, nullptr);
    }
    return LV_OBJ_TREE_WALK_NEXT;
}
} // namespace M9FocusTheme
