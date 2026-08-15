#ifndef __UI_MENU_H
#define __UI_MENU_H

#include "lvgl.h"

/* 菜单界面：三张卡片（上=current->prev / 中=current / 下=current->next） */
lv_obj_t *CreateMENU(void);             /* 创建菜单界面并返回屏对象 */
void Menu_Render(lv_obj_t *scr);        /* 按 menu_control.current 重绘三张卡片 */
lv_obj_t *Menu_Button_Create(lv_obj_t *parent,
                             lv_coord_t x, lv_coord_t y,
                             lv_coord_t w, lv_coord_t h);   /* 透明点击按钮（可选） */

#endif /* __UI_MENU_H */
