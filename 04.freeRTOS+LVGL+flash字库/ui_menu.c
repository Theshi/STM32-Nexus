#include "stm32f10x.h"                  // Device header
#include "SysTick.h"

#include "FreeRTOS.h"
#include "task.h"

#include "bsp_usart.h"
#include "bsp_ili9341_lcd.h"

#include "lvgl.h"
#include "lv_port_disp_template.h"
#include "lv_port_indev_template.h"

#include "bsp_spi_flash.h"
#include "flash_resource.h"

/* 外部字库接口（XBF 格式）：glyphs 从 W25Q64 SPI flash 实时读取，见 User/my_font_*.c */
extern const lv_font_t my_font_SCH_16;
extern const lv_font_t my_font_ENG_BT_16;
extern const lv_font_t my_font_ENG_UI_16;
extern const lv_font_t my_font_ENG_UI_28;
extern const lv_font_t my_font_ENG_UI_36;

/*尝试用OOP的思想与链表来实现整个菜单结构*/
/*————————————————————————————————Menu模块：只负责菜单管理————————————————————————————————————*/
typedef struct Menu
{   
    //选项的名称
    const char *name;
    //链表操作关系
    struct Menu * parent;//当前菜单的父菜单
    struct Menu *child;//当前菜单的子菜单
    struct Menu *next;//当前菜单中下一个选项
    struct Menu *prev;//当前菜单中上一个选项
    
    void(*select)(void);//选择当前菜单也就是进入这个功能选项

}MENU;
/*——————————————————————————————fanction模块：只负责每个菜单选项的功能实现—————————————————————*/
typedef struct App_function
{
    void(*run)();//运行当前功能
}APP_FUNCTION;
//选择节点的结构体
typedef struct 
{
    MENU *current;
    MENU *home;
}MENU_CONTROL;//初始化的时候直接给里面的指针指向一个节点就行
/*——————————————————————————————菜单界面（链表结构）——————————————————————————————————————————*/
//alarm_app节点以及功能
extern MENU alarm_app;
extern MENU game_app;
extern MENU seting_app;
extern MENU folder_app;
extern MENU music_app;
extern MENU fan_app;
extern MENU parperplae_app;

extern MENU_CONTROL menu_control;//选择节点（菜单）
MENU_CONTROL menu_control={
    .current=NULL,
    .home=NULL,
};

MENU alarm_app={

    .name="alarm",
    .parent=NULL,
    .child=NULL,
    .next=&game_app,
    .prev=&parperplae_app,

    .select=NULL,//alarm_app_run,//选择当前菜单也就是进入这个功能选项--------暂时先空着

};
MENU game_app={
    .name="game",
    .parent=NULL,
    .child=NULL,
    .next=&seting_app,
    .prev=&alarm_app,

    .select=NULL,//alarm_app_run,//选择当前菜单也就是进入这个功能选项--------暂时先空着
};
MENU seting_app={
    .name="seting",
    .parent=NULL,
    .child=NULL,
    .next=&folder_app,
    .prev=&game_app,

    .select=NULL,//alarm_app_run,//选择当前菜单也就是进入这个功能选项--------暂时先空着
};
MENU folder_app={
    .name="folder",
    .parent=NULL,
    .child=NULL,
    .next=&music_app,
    .prev=&seting_app,

    .select=NULL,//alarm_app_run,//选择当前菜单也就是进入这个功能选项--------暂时先空着
};
MENU music_app={
    .name="music",
    .parent=NULL,
    .child=NULL,
    .next=&fan_app,
    .prev=&folder_app,

    .select=NULL,//alarm_app_run,//选择当前菜单也就是进入这个功能选项--------暂时先空着
};
MENU fan_app={
    .name="fan",
    .parent=NULL,
    .child=NULL,
    .next=&parperplae_app,
    .prev=&music_app,

    .select=NULL,//alarm_app_run,//选择当前菜单也就是进入这个功能选项--------暂时先空着
};
MENU parperplae_app={
    .name="parperplane",
    .parent=NULL,
    .child=NULL,
    .next=&alarm_app,
    .prev=&fan_app,

    .select=NULL,//alarm_app_run,//选择当前菜单也就是进入这个功能选项--------暂时先空着
};

static void Menu_Button_Event(lv_event_t *e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED)
    {
        MENU *alarm_app = lv_event_get_user_data(e);

        if(alarm_app->select)
        {
            alarm_app->select();
        }
    }
}

//初始状态的菜单选项
void Menu_Init()
{
    menu_control.current=&alarm_app;
    menu_control.home=&alarm_app;
}
//下一个app
void Menu_Next()
{
    menu_control.current =
        menu_control.current->next;
}
//上一个app
void Menu_Prev()
{
    menu_control.current =
        menu_control.current->prev;
}
//选择当前app
void Menu_Select(void)
{
    if(menu_control.current->select)
    {
        menu_control.current->select();
    }
}
/*——————————————————————————————LVGL_UI模块：只负责UI的显示———————————————————————————————————*/
/*-----------------------启动界面-----------------*/
static lv_obj_t *label_log;              /* 自检日志 label      */
static char       boot_log[512];       /* 自检日志缓冲区       */
static lv_obj_t *bar_boot;            /* 进度条               */
static lv_obj_t *label_percent;       /* 进度百分比文字       */

//BootLog_Add —— 追加一条自检日志 
void BootLog_Add(const char *msg)
{
    strcat(boot_log, msg);
    strcat(boot_log, "\n");
    lv_label_set_text(label_log, boot_log);
}

// BootProgress_Set —— 更新进度条 + 百分比文字
void BootProgress_Set(uint8_t percent)
{
    lv_bar_set_value(bar_boot, percent, LV_ANIM_ON);
    lv_label_set_text_fmt(label_percent, "%d%%", percent);
}
//启动界面实现程序
void Boot_UI(void)
{   
     lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_white(), LV_PART_MAIN);

    // ZenStones Logo 
    lv_obj_t *label_Zen = lv_label_create(scr);
    lv_label_set_text(label_Zen, "NEXUS");
    lv_obj_set_style_text_font(label_Zen, &my_font_ENG_UI_36, LV_PART_MAIN);
    lv_obj_set_pos(label_Zen, 50, 30);

    //BOOTing 提示
    lv_obj_t *label_load = lv_label_create(scr);
    lv_label_set_text(label_load, "BOOTing ...");
    lv_obj_set_style_text_font(label_load, &my_font_ENG_BT_16, LV_PART_MAIN);
    lv_obj_set_pos(label_load, 27, 97);

    //进度条
    bar_boot = lv_bar_create(scr);
    lv_obj_set_size(bar_boot, 200, 15);
    lv_obj_set_pos(bar_boot, 27, 117);
    lv_obj_set_style_bg_color(bar_boot, lv_color_black(), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(bar_boot,
                              lv_palette_main(LV_PALETTE_BLUE),
                              LV_STATE_DEFAULT);
    lv_bar_set_mode(bar_boot, LV_BAR_MODE_NORMAL);
    lv_bar_set_range(bar_boot, 0, 100);
    lv_obj_set_style_anim_time(bar_boot, 500, LV_STATE_DEFAULT);
    lv_bar_set_value(bar_boot, 0, LV_ANIM_OFF);

    //进度百分比 
    label_percent = lv_label_create(scr);
    lv_label_set_text(label_percent, "0%");
    lv_obj_set_style_text_font(label_percent, &my_font_ENG_UI_16, LV_PART_MAIN);
    lv_obj_set_pos(label_percent, 230, 117);

    //自检日志区域 
    label_log = lv_label_create(scr);
    lv_obj_set_style_text_font(label_log, &my_font_ENG_UI_16, LV_PART_MAIN);
    lv_obj_set_pos(label_log, 27, 150);

    lv_obj_t *code_icon =ui_icon("code",scr ,LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_obj_t *terminal_icon=ui_icon("terminal",scr,  LV_ALIGN_TOP_RIGHT, -10, 50);
    lv_obj_t *desk_icon=ui_icon("desktop", scr, LV_ALIGN_TOP_RIGHT, -10, 90);

    //初始化日志缓冲区 
    memset(boot_log, 0, sizeof(boot_log));

}

/*-----------------------MENU主界面-----------------*/
//menu按钮的点击事件处理函数
static void btn_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    
    if(code == LV_EVENT_CLICKED)
    {
        //这里写切换到菜单界面的逻辑
        //后续添加进来menu的界面函数
        printf("menu button clicked\n");

    }
}
//refresh按钮的点击事件处理函数
static void refresh_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    
    if(code == LV_EVENT_CLICKED)
    {
        //这里写刷新界面的逻辑
        printf("refresh button clicked\n");
    }
}
//power按钮的点击事件处理函数
static void power_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    
    if(code == LV_EVENT_CLICKED)
    {
        //这里写关机的逻辑
        printf("power button clicked\n");
    }
}
void LVGL_CreateMainScreen(lv_obj_t *screen)
{
    // 设置背景颜色
    lv_obj_set_style_bg_color(screen, lv_color_white(), LV_PART_MAIN);

    /* ---- 时间显示 "12:00" （后续用 RTC 数据替换） ---- */
    lv_obj_t *Time_Label = lv_label_create(screen);
    lv_label_set_text(Time_Label, "12:00");
    lv_obj_set_style_text_font(Time_Label, &my_font_ENG_UI_36, LV_PART_MAIN);
    lv_obj_set_pos(Time_Label, 80, 50);

    // 日期显示 "2024-06-01" （后续用 RTC 数据替换）
    lv_obj_t *Date_Label = lv_label_create(screen);
    lv_label_set_text(Date_Label, "2025-06-01");
    lv_obj_set_style_text_font(Date_Label, &my_font_ENG_UI_16, LV_PART_MAIN);
    lv_obj_set_pos(Date_Label, 100, 100);

    lv_obj_t *Week_Lable=lv_label_create(screen);
    lv_obj_set_style_text_font(Week_Lable,&my_font_ENG_BT_16,LV_PART_MAIN);
    lv_obj_set_pos(Date_Label, 50, 100);
    
    //menu按钮--------菜单界面的启动按钮
    lv_obj_t *btn_menu = lv_btn_create(screen);
    lv_obj_set_size(btn_menu,50,50);
    lv_obj_align(btn_menu, LV_ALIGN_CENTER, 0, 0);
    lv_obj_t *menu_icon = ui_icon("menu",screen, LV_ALIGN_CENTER, 0, 0);
    lv_obj_center(menu_icon);

    lv_obj_add_event_cb(btn_menu, btn_event_cb, LV_EVENT_CLICKED, NULL);

    // 其他 UI 元素可以在这里添加、
    lv_obj_t *refresh_icon = ui_icon("refresh", screen,LV_ALIGN_CENTER, 0, 0);
    lv_obj_t *power_icon= ui_icon("power",screen,LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(refresh_icon,refresh_event_cb,LV_EVENT_CLICKED,NULL);

    //创建三个按钮作为选择与确定，直接放在图标覆盖的位置，设置为透明
    lv_obj_t *btn_confirm;
    lv_obj_t *btn_next;
    lv_obj_t *btn_prev;

    btn_confirm=Menu_Button_Create(screen,100,100,10,10);
    btn_next=Menu_Button_Create(screen,100,100,10,10);
    btn_prev=Menu_Button_Create(screen,100,100,10,10);


}
//创建按钮的封装函数
lv_obj_t *Menu_Button_Create(lv_obj_t *parent,
                             lv_coord_t x,
                             lv_coord_t y,
                             lv_coord_t w,
                             lv_coord_t h)
{
    lv_obj_t *btn;
    /* 创建按钮 */
    btn = lv_btn_create(parent);
    /* 设置按钮大小 */
    lv_obj_set_size(btn, w, h);
    /* 设置按钮位置 */
    lv_obj_set_pos(btn, x, y);
    /* 设置为透明 */
    lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
    /* 去掉边框 */
    lv_obj_set_style_border_width(btn, 0, 0);
    /* 去掉阴影 */
    lv_obj_set_style_shadow_width(btn, 0, 0);
    return btn;

}
//UI界面的切换-----------------后续改成从自检到menu两个界面的切换
void Boot_Finish(lv_obj_t *boot_screen, lv_obj_t *main_screen)
{
    /* 填充 Main Screen 内容 */
    LVGL_CreateMainScreen(main_screen);

    /* 带淡入动画切屏 */
    lv_scr_load_anim(main_screen,
                     LV_SCR_LOAD_ANIM_FADE_ON,
                     500,
                     0,
                     false);

    /* 删除 Boot Screen，释放内存 */
    lv_obj_del(boot_screen);
    printf("UI: 自检完成, 切换到主界面\n");
}
