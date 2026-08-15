#include "ui_screen.h"
#include "lvgl.h"

#include <stdio.h>

/* TODO(用户): 集中式屏幕管理器，三步走：
 *   1. 删除当前屏对象，并释放该屏所有图标缓冲（补 ui_icon 的泄漏）
 *   2. 按 next 调用对应创建函数拿到新屏：Boot_UI() / LVGL_CreateMainScreen() / CreateMENU()
 *   3. lv_scr_load(新屏)，可带淡入动画（参考原 Boot_Finish 的 lv_scr_load_anim 写法）
 *
 * 原 Boot_Finish（删 boot + 建 main + 切屏）已并入此入口，不再单独保留。
 */
void Screen_Switch(SCREEN_ID next)
{
    (void)next;
    /* TODO */
}
