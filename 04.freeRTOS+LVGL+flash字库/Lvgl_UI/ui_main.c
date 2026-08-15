/* 主界面：时间/日期显示 + menu 按钮 + 状态图标（填充传入的 screen）
 * 由 ui_screen.c 的 Screen_Switch(SCREEN_MAIN) 调用创建 */
#include "ui_main.h"
#include "flash_resource.h"
#include "ui_screen.h"          /* TODO: 切换到菜单界面时需要 Screen_Switch */

#include <stdio.h>

/* 外部字库接口（XBF 格式）：glyphs 从 W25Q64 SPI flash 实时读取 */
extern const lv_font_t my_font_ENG_BT_16;
extern const lv_font_t my_font_ENG_UI_16;
extern const lv_font_t my_font_ENG_UI_36;

/* menu 按钮的点击事件处理函数 */
static void btn_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED)
    {
        /* TODO: 切换到菜单界面 → Screen_Switch(SCREEN_MENU) */
        printf("menu button clicked\n");
    }
}
/* refresh 按钮的点击事件处理函数 */
static void refresh_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED)
    {
        /* TODO: 刷新界面的逻辑 */
        printf("refresh button clicked\n");
    }
}
/* power 按钮的点击事件处理函数 */
static void power_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED)
    {
        /* TODO: 关机的逻辑 */
        printf("power button clicked\n");
    }
}

void LVGL_CreateMainScreen(lv_obj_t *screen)
{
    /* 设置背景颜色 */
    lv_obj_set_style_bg_color(screen, lv_color_white(), LV_PART_MAIN);

    /* ---- 时间显示 "12:00" （后续用 RTC 数据替换） ---- */
    lv_obj_t *Time_Label = lv_label_create(screen);
    lv_label_set_text(Time_Label, "12:00");
    lv_obj_set_style_text_font(Time_Label, &my_font_ENG_UI_36, LV_PART_MAIN);
    lv_obj_set_pos(Time_Label, 80, 50);

    /* ---- 日期显示 "2025-06-01" （后续用 RTC 数据替换） ---- */
    lv_obj_t *Date_Label = lv_label_create(screen);
    lv_label_set_text(Date_Label, "2025-06-01");
    lv_obj_set_style_text_font(Date_Label, &my_font_ENG_UI_16, LV_PART_MAIN);
    lv_obj_set_pos(Date_Label, 100, 100);

    /* ---- 星期显示（文本内容后续补） ---- */
    lv_obj_t *Week_Lable = lv_label_create(screen);
    lv_obj_set_style_text_font(Week_Lable, &my_font_ENG_BT_16, LV_PART_MAIN);
    lv_obj_set_pos(Week_Lable, 50, 100);   /* 原代码误用 Date_Label 定位，已改为 Week_Lable */

    /* menu 按钮 -------- 菜单界面的启动按钮 */
    lv_obj_t *btn_menu = lv_btn_create(screen);
    lv_obj_set_size(btn_menu, 50, 50);
    lv_obj_align(btn_menu, LV_ALIGN_CENTER, 0, 0);
    lv_obj_t *menu_icon = ui_icon("menu", screen, LV_ALIGN_CENTER, 0, 0);
    lv_obj_center(menu_icon);

    lv_obj_add_event_cb(btn_menu, btn_event_cb, LV_EVENT_CLICKED, NULL);

    /* 其他 UI 元素可以在这里添加 */
    lv_obj_t *refresh_icon = ui_icon("refresh", screen, LV_ALIGN_CENTER, 0, 0);
    lv_obj_t *power_icon   = ui_icon("power",   screen, LV_ALIGN_CENTER, 0, 0);

    /* 图标兼做按钮：先加 CLICKABLE 才能收到点击事件（原代码 power 未挂事件且没加 CLICKABLE，已补） */
    lv_obj_add_flag(refresh_icon, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(power_icon,   LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(refresh_icon, refresh_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(power_icon,   power_event_cb,   LV_EVENT_CLICKED, NULL);
}
