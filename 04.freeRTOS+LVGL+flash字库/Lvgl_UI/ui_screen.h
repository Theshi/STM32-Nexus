#ifndef __UI_SCREEN_H
#define __UI_SCREEN_H

/*——————————————————————————————————————————————
 * 集中式屏幕管理器：所有界面切换的唯一入口
 * 三个屏幕按需创建销毁，任一时刻只有一屏存活
 * 详见 docs/superpowers/specs/2026-08-14-menu-screen-architecture-design.md
 *——————————————————————————————————————————————*/

typedef enum
{
    SCREEN_BOOT,    /* 启动自检界面 */
    SCREEN_MAIN,    /* 主界面：时间/日期 + menu 按钮 */
    SCREEN_MENU,    /* 菜单界面：三张卡片 */
} SCREEN_ID;

void Screen_Switch(SCREEN_ID next);

#endif /* __UI_SCREEN_H */
