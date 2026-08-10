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
	/* 放到屏幕上方，避免挡住居中的图标测试 */
	lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 20);
}

/* 图标显示测试：从 Flash 读图标 bin（4B 头 + RGB565 像素）并显示
 * icon_buf/dsc 必须 static：lv_img_set_src 引用结构体与数据指针，不会拷贝 */
void LVGL_Icon_Test(const char *name){
	static uint8_t icon_buf[8192];          /* 最大图标 60×60×2+4 = 7204B */
	static lv_img_dsc_t dsc;
	uint32_t addr, size, hdr, cf, w, h;

	if(!Flash_FindResource(name, &addr, &size)){
		printf("icon [%s] not found!\r\n", name);
		return;
	}
	printf("read icon [%s]: addr=0x%06X size=%d\r\n", name, addr, size);
	SPI_FLASH_BufferRead(icon_buf, addr, (uint16_t)((size > sizeof(icon_buf)) ? sizeof(icon_buf) : size));

	/* 解析 4 字节打包位域（小端） */
	hdr = icon_buf[0] | (icon_buf[1]<<8) | (icon_buf[2]<<16) | ((uint32_t)icon_buf[3]<<24);
	cf  = hdr & 0x1F;                        /* 应为 4 = LV_IMG_CF_TRUE_COLOR */
	w   = (hdr >> 10) & 0x7FF;
	h   = (hdr >> 21) & 0x7FF;
	printf("icon hdr: cf=%d w=%d h=%d data=%d\r\n", cf, w, h, w*h*2);

	dsc.header.always_zero = 0;
	dsc.header.cf    = LV_IMG_CF_TRUE_COLOR;
	dsc.header.w     = w;
	dsc.header.h     = h;
	dsc.data_size    = w*h*2;
	dsc.data         = &icon_buf[4];

	lv_obj_t * img = lv_img_create(lv_scr_act());
	lv_img_set_src(img, &dsc);
	/* 移到右上角，避免挡住下面的字库显示测试 */
	lv_obj_align(img, LV_ALIGN_TOP_RIGHT, -8, 60);
}

/* 字库显示测试：中英文都从 W25Q64 实时读字模（__user_font_getdata -> flash_font_getdata）。
 * 注意：烧录的字库不含空格(0x20)，英文空格由 fallback（montserrat）提供；
 *      中文测试串选用字库内含的字（"你好世界" 不在烧录的字符集里）。 */
void LVGL_Font_Test(void){
	static const struct {
		const char *txt;
		const lv_font_t *f;
		int y;
	} rows[] = {
		/* 屏幕是 320x240 横屏，5 行全部排在 240 高度内（行高 26/28/47/61/29） */
		{ "hello world", &my_font_ENG_BT_16,  12 },
		{ "hello world", &my_font_ENG_UI_16,  42 },
		{ "hello",       &my_font_ENG_UI_28,  76 },
		{ "hello",       &my_font_ENG_UI_36, 130 },
		{ "数据显示",     &my_font_SCH_16,    200 },
	};
	uint8_t i;
	for(i = 0; i < sizeof(rows)/sizeof(rows[0]); i++){
		lv_obj_t * lb = lv_label_create(lv_scr_act());
		lv_label_set_text(lb, rows[i].txt);
		lv_obj_set_style_text_font(lb, rows[i].f, 0);
		lv_obj_align(lb, LV_ALIGN_TOP_LEFT, 10, rows[i].y);
	}
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
	Flash_Index_Test();

	LVGL_Test();
	/* 诊断：查看各区域真实内容，确定擦除范围 */
	Flash_AreaDump(0x300000, 512);   /* alarm 图标区 */
	Flash_AreaDump(0x302000, 512);   /* refresh 图标区 */
	Flash_AreaDump(0x304000, 512);   /* battery 图标区 */
	Flash_AreaDump(0x308000, 512);   /* left 图标区 */
	Flash_AreaDump(0x002000, 16);    /* sch16 字库 XBF 头 */
	Flash_AreaDump(0x08D000, 16);    /* eng_bt16 字库 XBF 头 */
	/* Flash 内容已确认完整，启用图标显示测试（alarm 50x50 右上角） */
	LVGL_Icon_Test("alarm");
	/* 字库显示测试：英文 4 款 + 中文字库，全部从 W25Q64 读字模 */
	LVGL_Font_Test();
	printf("屏幕初始化完毕..\n");
	
	SysTick_Init();
	xTaskCreate(StartUpTask,"StartUpTask",StartUpTask_STACKSIZE,NULL,StartUpTask_PRIO,&StartUpTask_Handle);
	xTaskCreate(LvglTask,"LvglTask",LvglTask_STACKSIZE,NULL,LvglTask_PRIO,&LvglTask_Handle);
	vTaskStartScheduler();
}

