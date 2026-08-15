/* 启动自检界面：LOGO + 进度条 + 自检日志 + 三个角标图标
 * 由 ui_screen.c 的 Screen_Switch(SCREEN_BOOT) 调用创建 */
#include "ui_boot.h"
#include "flash_resource.h"

#include <string.h>
#include <stdio.h>

/* 外部字库接口（XBF 格式）：glyphs 从 W25Q64 SPI flash 实时读取 */
extern const lv_font_t my_font_ENG_BT_16;
extern const lv_font_t my_font_ENG_UI_16;
extern const lv_font_t my_font_ENG_UI_36;

static lv_obj_t *label_log;         /* 自检日志 label      */
static char       boot_log[512];    /* 自检日志缓冲区       */
static lv_obj_t *bar_boot;          /* 进度条              */
static lv_obj_t *label_percent;     /* 进度百分比文字       */

/* BootLog_Add —— 追加一条自检日志 */
void BootLog_Add(const char *msg)
{
    strcat(boot_log, msg);
    strcat(boot_log, "\n");
    lv_label_set_text(label_log, boot_log);
}

/* BootProgress_Set —— 更新进度条 + 百分比文字 */
void BootProgress_Set(uint8_t percent)
{
    lv_bar_set_value(bar_boot, percent, LV_ANIM_ON);
    lv_label_set_text_fmt(label_percent, "%d%%", percent);
}

/* 启动界面实现程序：创建界面并返回屏对象（供 Screen_Switch 持有/销毁） */
lv_obj_t *Boot_UI(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_white(), LV_PART_MAIN);

    /* ZenStones Logo */
    lv_obj_t *label_Zen = lv_label_create(scr);
    lv_label_set_text(label_Zen, "NEXUS");
    lv_obj_set_style_text_font(label_Zen, &my_font_ENG_UI_36, LV_PART_MAIN);
    lv_obj_set_pos(label_Zen, 50, 30);

    /* BOOTing 提示 */
    lv_obj_t *label_load = lv_label_create(scr);
    lv_label_set_text(label_load, "BOOTing ...");
    lv_obj_set_style_text_font(label_load, &my_font_ENG_BT_16, LV_PART_MAIN);
    lv_obj_set_pos(label_load, 27, 97);

    /* 进度条 */
    bar_boot = lv_bar_create(scr);
    lv_obj_set_size(bar_boot, 200, 15);
    lv_obj_set_pos(bar_boot, 27, 117);
    lv_obj_set_style_bg_color(bar_boot, lv_color_black(), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(bar_boot, lv_palette_main(LV_PALETTE_BLUE), LV_STATE_DEFAULT);
    lv_bar_set_mode(bar_boot, LV_BAR_MODE_NORMAL);
    lv_bar_set_range(bar_boot, 0, 100);
    lv_obj_set_style_anim_time(bar_boot, 500, LV_STATE_DEFAULT);
    lv_bar_set_value(bar_boot, 0, LV_ANIM_OFF);

    /* 进度百分比 */
    label_percent = lv_label_create(scr);
    lv_label_set_text(label_percent, "0%");
    lv_obj_set_style_text_font(label_percent, &my_font_ENG_UI_16, LV_PART_MAIN);
    lv_obj_set_pos(label_percent, 230, 117);

    /* 自检日志区域 */
    label_log = lv_label_create(scr);
    lv_obj_set_style_text_font(label_log, &my_font_ENG_UI_16, LV_PART_MAIN);
    lv_obj_set_pos(label_log, 27, 150);

    lv_obj_t *code_icon     = ui_icon("code",     scr, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_obj_t *terminal_icon = ui_icon("terminal", scr, LV_ALIGN_TOP_RIGHT, -10, 50);
    lv_obj_t *desk_icon     = ui_icon("desktop",  scr, LV_ALIGN_TOP_RIGHT, -10, 90);

    /* 初始化日志缓冲区 */
    memset(boot_log, 0, sizeof(boot_log));

    return scr;
}
