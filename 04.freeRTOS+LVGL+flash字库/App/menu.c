#include "menu.h"
#include "app.h"

#include <stddef.h>   /* NULL */

/* 全局导航状态 */
MENU_CONTROL menu_control = {
    .current = NULL,
    .home    = NULL,
};

/* 7 个 app 节点：环形链表（next/prev 首尾相连） */
MENU alarm_app = {
    .name   = "alarm_app",
    .icon   = "alarm",
    .parent = NULL,
    .child  = NULL,
    .next   = &game_app,
    .prev   = &parperplae_app,
    .select = App_Alarm_Run,
};
MENU game_app = {
    .name   = "game_app",
    .icon   = "gamep",
    .parent = NULL,
    .child  = NULL,
    .next   = &seting_app,
    .prev   = &alarm_app,
    .select = App_Game_Run,
};
MENU seting_app = {
    .name   = "seting_app",
    .icon   = "set",
    .parent = NULL,
    .child  = NULL,
    .next   = &folder_app,
    .prev   = &game_app,
    .select = App_Setting_Run,
};
MENU folder_app = {
    .name   = "folder_app",
    .icon   = "folder",
    .parent = NULL,
    .child  = NULL,
    .next   = &music_app,
    .prev   = &seting_app,
    .select = App_Folder_Run,
};
MENU music_app = {
    .name   = "music_app",
    .icon   = "music",
    .parent = NULL,
    .child  = NULL,
    .next   = &fan_app,
    .prev   = &folder_app,
    .select = App_Music_Run,
};
MENU fan_app = {
    .name   = "fan_app",
    .icon   = "fan",
    .parent = NULL,
    .child  = NULL,
    .next   = &parperplae_app,
    .prev   = &music_app,
    .select = App_Fan_Run,
};
MENU parperplae_app = {
    .name   = "parperplae_app",
    .icon   = "paperplane",
    .parent = NULL,
    .child  = NULL,
    .next   = &alarm_app,
    .prev   = &fan_app,
    .select = App_Paperplane_Run,
};

/* 初始状态：current/home 都指向第一个节点 */
void Menu_Init(void)
{
    menu_control.current = &alarm_app;
    menu_control.home    = &alarm_app;
}
/* 下一个 app */
void Menu_Next(void)
{
    menu_control.current = menu_control.current->next;
}
/* 上一个 app */
void Menu_Prev(void)
{
    menu_control.current = menu_control.current->prev;
}
/* 选择当前 app（执行其功能） */
void Menu_Select(void)
{
    if(menu_control.current->select)
    {
        menu_control.current->select();
    }
}
