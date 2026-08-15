#ifndef __UI_BOOT_H
#define __UI_BOOT_H

#include "lvgl.h"

/* 启动自检界面：LOGO + 进度条 + 自检日志 + 角标图标 */
lv_obj_t *Boot_UI(void);                  /* 创建自检界面并返回屏对象 */
void BootLog_Add(const char *msg);        /* 追加一条自检日志 */
void BootProgress_Set(uint8_t percent);   /* 更新进度条 + 百分比文字 */

#endif /* __UI_BOOT_H */
