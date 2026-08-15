#ifndef __MENU_H
#define __MENU_H

/*——————————————————————————————————————————————
 * Menu 模块：链表式菜单管理（只负责菜单数据结构与导航）
 * 一个节点 = 一个 app 选项，7 个节点首尾相连成环形链表
 * 纯 C 逻辑，不依赖 LVGL
 *——————————————————————————————————————————————*/

typedef struct Menu
{
    const char *name;        /* 选项名称（app 名） */
    const char *icon;        /* 图标资源名（W25Q64 里的 .bin 名，如 "alarm"、"gamep"） */
    struct Menu *parent;     /* 父菜单（层级菜单用，当前为 NULL） */
    struct Menu *child;      /* 子菜单（层级菜单用，当前为 NULL） */
    struct Menu *next;       /* 下一个选项 */
    struct Menu *prev;       /* 上一个选项 */
    void(*select)(void);     /* 选中进入该 app 时执行的功能 */
} MENU;

/* 导航状态：current = 当前选中节点，home = 初始节点 */
typedef struct
{
    MENU *current;
    MENU *home;
} MENU_CONTROL;

/* 7 个 app 节点（定义在 menu.c） */
extern MENU alarm_app;
extern MENU game_app;
extern MENU seting_app;
extern MENU folder_app;
extern MENU music_app;
extern MENU fan_app;
extern MENU parperplae_app;

/* 全局导航状态（定义在 menu.c） */
extern MENU_CONTROL menu_control;

/* 导航 API */
void Menu_Init(void);       /* 初始化：current/home 指向 alarm_app */
void Menu_Next(void);       /* 下一个 */
void Menu_Prev(void);       /* 上一个 */
void Menu_Select(void);     /* 执行当前节点 select（进入该 app） */

#endif /* __MENU_H */
