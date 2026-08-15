#ifndef __APP_H
#define __APP_H

/*——————————————————————————————————————————————
 * App 功能模块：每个菜单选项对应的功能函数
 * 目前全部为空实现（printf 占位），后续逐个完善
 * 由 menu.c 里各节点的 select 指针挂到这些函数上
 *——————————————————————————————————————————————*/

void App_Alarm_Run(void);        /* 闹钟 */
void App_Game_Run(void);         /* 游戏 */
void App_Setting_Run(void);      /* 设置 */
void App_Folder_Run(void);       /* 文件/文件夹 */
void App_Music_Run(void);        /* 音乐 */
void App_Fan_Run(void);          /* 风扇 */
void App_Paperplane_Run(void);   /* 纸飞机 */

#endif /* __APP_H */
