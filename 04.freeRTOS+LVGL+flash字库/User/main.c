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

#define TEST_CODE 0//测试程序

//启动任务
#define StartUpTask_STACKSIZE 256
#define StartUpTask_PRIO			1
TaskHandle_t StartUpTask_Handle;
void StartUpTask(void * p);

//LVGL任务
#define LvglTask_STACKSIZE 512
#define LvglTask_PRIO			1
TaskHandle_t LvglTask_Handle;
void LvglTask(void * p);

//开启任务
void StartUpTask(void *p){
	while(1){
		vTaskDelay(1000);
		printf("run...\n");
	}
}
//LVGL任务
void LvglTask(void * p){
	while(1){
		vTaskDelay(5);
		lv_timer_handler();
	}
}

void LVGL_Test(){
	lv_obj_t * btn = lv_btn_create(lv_scr_act());
	lv_obj_t * label = lv_label_create(btn);
	lv_label_set_text(label,"hello");
	
	lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 20);
}


int main(){
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
	USART_Config();
	printf("窗口初始化完毕..\n");
	lv_init();
	lv_port_disp_init();
	/* 屏幕背景设为纯白（默认主题是浅灰），让白底图标与背景无缝融合 */
	lv_obj_set_style_bg_color(lv_scr_act(), lv_color_white(), 0);
	lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);
	/* 关闭根屏幕滚动：固定布局菜单，内容都在可视区内，避免误拖/惯性滑动 */
	lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);
	lv_port_indev_init();
	
	SPI_FLASH_Init();
	printf("ID:0x%X\n",SPI_FLASH_ReadID());
//	Flash_Index_Test();

	LVGL_Test();
#if TEST_CODE == 1
	/* 诊断：查看各区域真实内容，确定擦除范围 */
	Flash_AreaDump(0x300000, 512);   /* alarm 图标区 */
	Flash_AreaDump(0x302000, 512);   /* refresh 图标区 */
	Flash_AreaDump(0x304000, 512);   /* battery 图标区 */
	Flash_AreaDump(0x308000, 512);   /* left 图标区 */
	Flash_AreaDump(0x002000, 16);    /* sch16 字库 XBF 头 */
	Flash_AreaDump(0x08D000, 16);    /* eng_bt16 字库 XBF 头 */
#endif
	/* Flash 内容已确认完整，启用图标显示测试（alarm 50x50 右上角） */
//	LVGL_Icon_Test("alarm");
	/* 字库显示测试：英文 4 款 + 中文字库，全部从 W25Q64 读字模 */
//	LVGL_Font_Test();

	printf("屏幕初始化完毕..\n");
	
	SysTick_Init();
	xTaskCreate(StartUpTask,"StartUpTask",StartUpTask_STACKSIZE,NULL,StartUpTask_PRIO,&StartUpTask_Handle);
	xTaskCreate(LvglTask,"LvglTask",LvglTask_STACKSIZE,NULL,LvglTask_PRIO,&LvglTask_Handle);
	vTaskStartScheduler();
}

