/* 菜单界面：链表驱动，三卡片渲染 + 触摸导航
 *
 * 设计要点（详见 docs/superpowers/specs/2026-08-14-menu-screen-architecture-design.md）：
 *  - 渲染 Menu_Render()：上卡 = current->prev，中卡 = current，下卡 = current->next
 *    每卡 = 图标(ui_icon) + 名字 label；重渲染 = 清旧三卡 → 画新三卡
 *  - 交互：三张卡片本身即按钮（加 LV_OBJ_FLAG_CLICKABLE + 事件回调），不另建透明按钮
 *     上卡点按 → Menu_Prev() → Menu_Render()
 *     下卡点按 → Menu_Next() → Menu_Render()
 *     中卡点按 → Menu_Select()（进入该 app）
 *  - 返回按钮 → Screen_Switch(SCREEN_MAIN)
 *  - 注意：ui_icon 每次会分配图标缓冲，删屏/重渲染时必须释放，否则堆会漏光（见设计文档待处理项 1）
 */
#include "ui_menu.h"
#include "menu.h"
#include "flash_resource.h"
#include "ui_screen.h"

#include <stdio.h>

/* 外部字库接口（XBF 格式）：glyphs 从 W25Q64 SPI flash 实时读取 */
extern const lv_font_t my_font_ENG_UI_16;

/* 创建透明按钮的封装函数（可选；按设计主张卡片本身即可点击，透明按钮只在需要隐藏点击区时用） */
lv_obj_t *Menu_Button_Create(lv_obj_t *parent,
                             lv_coord_t x,
                             lv_coord_t y,
                             lv_coord_t w,
                             lv_coord_t h)
{
    lv_obj_t *btn;
    btn = lv_btn_create(parent);
    lv_obj_set_size(btn, w, h);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);      /* 透明 */
    lv_obj_set_style_border_width(btn, 0, 0);             /* 去边框 */
    lv_obj_set_style_shadow_width(btn, 0, 0);             /* 去阴影 */
    return btn;
}

/* TODO(用户): 按 menu_control.current 渲染三张卡片
 *  - 上卡 = current->prev，中卡 = current，下卡 = current->next
 *  - 每卡 = ui_icon(节点->icon) + lv_label(节点->name)
 *  - 三卡都加 LV_OBJ_FLAG_CLICKABLE + 事件回调，点击卡片本身即可切换/进入
 *  - 重渲染前先把上一轮的三卡对象删掉（并释放对应图标缓冲），避免叠堆
 */
void Menu_Render(lv_obj_t *scr)
{
    (void)scr;
    /* TODO */
}

/* TODO(用户): 创建菜单界面
 *  1. 创建屏对象 + 白色背景
 *  2. Menu_Render(scr) 渲染三张卡片
 *  3. 补一个"返回"按钮 → Screen_Switch(SCREEN_MAIN)
 */
lv_obj_t *CreateMENU(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_white(), LV_PART_MAIN);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    Menu_Render(scr);   /* TODO(用户): 三卡渲染 + 卡片点击事件 + 返回按钮 */

    return scr;
}
