#include "graphics/view/TFT/M9FocusTheme.h"
#include "graphics/view/TFT/DropdownKeyNavigation.h"
#include <doctest/doctest.h>

TEST_CASE("light focus refresh recolors initial label and icon and restores on defocus")
{
    if (!lv_is_initialized())
        lv_init();
    if (!lv_display_get_default())
        lv_display_create(320, 240);
    auto *root = lv_obj_create(nullptr);
    auto *a = lv_button_create(root);
    auto *label = lv_label_create(a);
    lv_label_set_text(label, "Selected item");
    lv_obj_set_style_text_color(label, lv_color_hex(0x334155), 0);
    auto *icon = lv_image_create(a);
    lv_obj_set_style_image_recolor(icon, lv_color_black(), 0);
    auto *b = lv_button_create(root);
    auto *group = lv_group_create();
    lv_group_add_obj(group, a);
    lv_group_add_obj(group, b);
    Themes::set(Themes::eLight);
    lv_obj_tree_walk(root, M9FocusTheme::refresh, nullptr);
    CHECK(lv_color_eq(lv_obj_get_style_bg_color(a, LV_PART_MAIN), lv_color_black()));
    CHECK(lv_color_eq(lv_obj_get_style_text_color(label, LV_PART_MAIN), lv_color_white()));
    CHECK(lv_color_eq(lv_obj_get_style_image_recolor(icon, LV_PART_MAIN), lv_color_white()));
    CHECK(lv_obj_get_style_border_width(a, LV_PART_MAIN) == 2);
    CHECK(lv_color_eq(lv_obj_get_style_border_color(a, LV_PART_MAIN), lv_color_hex(0xffd400)));
    CHECK(lv_obj_get_style_outline_width(a, LV_PART_MAIN) == 0);
    lv_group_focus_obj(b);
    CHECK(lv_color_eq(lv_obj_get_style_text_color(label, LV_PART_MAIN), lv_color_hex(0x334155)));
    CHECK(lv_color_eq(lv_obj_get_style_image_recolor(icon, LV_PART_MAIN), lv_color_black()));
    CHECK(lv_color_eq(lv_obj_get_style_bg_color(a, LV_PART_MAIN), lv_color_white()));
    lv_obj_add_state(b, LV_STATE_CHECKED);
    lv_obj_send_event(b, LV_EVENT_VALUE_CHANGED, nullptr);
    lv_group_focus_obj(a);
    CHECK(lv_color_eq(lv_obj_get_style_bg_color(b, LV_PART_MAIN), lv_color_black()));
    lv_obj_remove_state(b, LV_STATE_CHECKED);
    lv_obj_send_event(b, LV_EVENT_VALUE_CHANGED, nullptr);
    CHECK(lv_color_eq(lv_obj_get_style_bg_color(b, LV_PART_MAIN), lv_color_white()));
    lv_group_focus_obj(b);
    // A theme change removes all light-only styles and descendant overrides.
    Themes::set(Themes::eDark);
    lv_obj_tree_walk(root, M9FocusTheme::refresh, nullptr);
    CHECK_FALSE(lv_obj_has_state(b, LV_STATE_USER_1));
    Themes::set(Themes::eLight);
    lv_obj_tree_walk(root, M9FocusTheme::refresh, nullptr);
    CHECK(lv_color_eq(lv_obj_get_style_bg_color(b, LV_PART_MAIN), lv_color_black()));
    lv_group_delete(group);
    lv_obj_delete(root);
}

#ifdef ELECROW_ThinkNode_M9
TEST_CASE("light dropdown selected row has black fill white text and yellow border")
{
    if (!lv_is_initialized())
        lv_init();
    if (!lv_display_get_default())
        lv_display_create(320, 240);
    Themes::set(Themes::eLight);
    auto *root = lv_obj_create(nullptr);
    auto *dropdown = lv_dropdown_create(root);
    lv_dropdown_set_options(dropdown, "First\nSecond");
    DropdownKeyNavigation::attach(root);
    lv_dropdown_open(dropdown);
    auto *list = lv_dropdown_get_list(dropdown);
    REQUIRE(list != nullptr);
    lv_obj_add_state(list, LV_STATE_CHECKED);
    CHECK(lv_color_eq(lv_obj_get_style_bg_color(list, LV_PART_SELECTED), lv_color_black()));
    CHECK(lv_color_eq(lv_obj_get_style_text_color(list, LV_PART_SELECTED), lv_color_white()));
    CHECK(lv_color_eq(lv_obj_get_style_border_color(list, LV_PART_SELECTED), lv_color_hex(0xffd400)));
    CHECK(lv_obj_get_style_border_width(list, LV_PART_SELECTED) == 2);
    lv_dropdown_close(dropdown);
    lv_obj_delete(root);
}

#endif

TEST_CASE("node row focus styles the outer row without styling the transparent hit target")
{
    if (!lv_is_initialized()) lv_init();
    if (!lv_display_get_default()) lv_display_create(320, 240);
    Themes::set(Themes::eLight);
    auto *root = lv_obj_create(nullptr);
    auto *row = lv_obj_create(root);
    auto *hit = lv_button_create(row);
    lv_obj_set_style_bg_opa(hit, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(hit, 0, LV_PART_MAIN);
    M9FocusTheme::attach(hit, row);
    auto *group = lv_group_create();
    lv_group_add_obj(group, hit);
    lv_obj_tree_walk(root, M9FocusTheme::refresh, nullptr);
    CHECK(lv_obj_get_style_border_width(hit, LV_PART_MAIN) == 0);
    CHECK(lv_obj_get_style_bg_opa(hit, LV_PART_MAIN) == LV_OPA_TRANSP);
    CHECK(lv_obj_get_style_border_width(row, LV_PART_MAIN) == 2);
    CHECK(lv_color_eq(lv_obj_get_style_border_color(row, LV_PART_MAIN), lv_color_hex(0xffd400)));
    lv_group_delete(group);
    lv_obj_delete(root);
}
