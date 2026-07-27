#include "stm32f10x.h"                  // Device header
#include "SysTick.h"

#include "FreeRTOS.h"
#include "task.h"

#include "bsp_usart.h"
#include "bsp_ili9341_lcd.h"

#include "lvgl.h"
#include "lv_port_disp_template.h"
#include "lv_port_indev_template.h"

#include "bsp_sdio_sdcard.h"
#include "ffconf.h"
#include "ff.h"

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

#define FatFsTask_STACKSIZE 1024
#define FatFsTask_PRIO			1
TaskHandle_t FATFStask_Handle;
void FatFsTask(void * p);

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

//文件操作系统参数
FATFS fs;													/* FatFs文件系统对象 */
FIL fnew;													/* 文件对象 */
FRESULT res_sd;                /* 文件操作结果 */
UINT fnum;            					  /* 文件成功读写数量 */
BYTE ReadBuffer[1024]={0};        /* 读缓冲区 */
BYTE WriteBuffer[] =              /* 写缓冲区*/
"欢迎使用野火STM32 开发板 今天是个好日子，新建文件系统测试文件\r\n";
static BYTE work_buffer[512];	/* 文件系统工作缓冲区（至少 FF_MAX_SS=512 字节） */
extern  SD_CardInfo SDCardInfo;
//fastfs文件系统测试任务
void FatFsTask(void *p)
{
	/* 初始化调试串口，一般为串口1 */
	USART_Config();	
  printf("\r\n****** 这是一个SD卡 文件系统实验 ******\r\n");
  
	//在外部SPI Flash挂载文件系统，文件系统挂载时会对SPI设备初始化
	res_sd = f_mount(&fs,"0:",1);
	
//	printf("容量=%lld",SDCardInfo.CardCapacity/1024/1024);
/*----------------------- 格式化测试 ---------------------------*/  
	/* 如果没有文件系统就格式化创建创建文件系统 */
	if(res_sd == FR_NO_FILESYSTEM)
	{
		printf("》SD卡还没有文件系统，即将进行格式化...\r\n");
			/* 格式化 — FatFS R0.15 使用 MKFS_PARM 结构体传参 */
			MKFS_PARM opt = {
				.fmt = FM_FAT32,	/* FAT32 格式 */
				.n_fat = 0,			/* 0=默认（2个FAT表） */
				.align = 0,			/* 0=默认对齐 */
				.n_root = 0,		/* 0=默认根目录条目数 */
				.au_size = 0		/* 0=默认簇大小 */
			};
			res_sd = f_mkfs("0:", &opt, work_buffer, sizeof(work_buffer));
		
		if(res_sd == FR_OK)
		{
			printf("》SD卡已成功格式化文件系统。\r\n");
      /* 格式化后，先取消挂载 */
			res_sd = f_mount(NULL,"0:",1);			
      /* 重新挂载	*/			
			res_sd = f_mount(&fs,"0:",1);
		}
		else
		{
			
			printf("《《格式化失败。》》\r\n");
			while(1);
		}
	}
  else if(res_sd!=FR_OK)
  {
    printf("！！SD卡挂载文件系统失败。(%d)\r\n",res_sd);
    printf("！！可能原因：SD卡初始化不成功。\r\n");
		while(1);
  }
  else
  {
    printf("》文件系统挂载成功，可以进行读写测试\r\n");
  }
  
/*----------------------- 文件系统测试：写测试 -----------------------------*/
	/* 打开文件，如果文件不存在则创建它 */
	printf("\r\n****** 即将进行文件写入测试... ******\r\n");	
	res_sd = f_open(&fnew, "0:FatFs读写测试文件.txt",FA_CREATE_ALWAYS | FA_WRITE );
	if ( res_sd == FR_OK )
	{
		printf("》打开/创建FatFs读写测试文件.txt文件成功，向文件写入数据。\r\n");
    /* 将指定存储区内容写入到文件内 */
		res_sd=f_write(&fnew,WriteBuffer,sizeof(WriteBuffer),&fnum);
    if(res_sd==FR_OK)
    {
      printf("》文件写入成功，写入字节数据：%d\n",fnum);
      printf("》向文件写入的数据为：\r\n%s\r\n",WriteBuffer);
    }
    else
    {
      printf("！！文件写入失败：(%d)\n",res_sd);
    }    
		/* 不再读写，关闭文件 */
    f_close(&fnew);
	}
	else
	{	
		
		printf("！！打开/创建文件失败。\r\n");
	}
	
/*------------------- 文件系统测试：读测试 ------------------------------------*/
	printf("****** 即将进行文件读取测试... ******\r\n");
	res_sd = f_open(&fnew, "0:FatFs读写测试文件.txt", FA_OPEN_EXISTING | FA_READ); 	 
	if(res_sd == FR_OK)
	{
		
		printf("》打开文件成功。\r\n");
		res_sd = f_read(&fnew, ReadBuffer, sizeof(ReadBuffer), &fnum); 
    if(res_sd==FR_OK)
    {
      printf("》文件读取成功,读到字节数据：%d\r\n",fnum);
      printf("》读取得的文件数据为：\r\n%s \r\n", ReadBuffer);	
    }
    else
    {
      printf("！！文件读取失败：(%d)\n",res_sd);
    }		
	}
	else
	{
		
		printf("！！打开文件失败。\r\n");
	}
	/* 不再读写，关闭文件 */
	f_close(&fnew);	
  
	/* 不再使用文件系统，取消挂载文件系统 */
	f_mount(NULL,"0:",1);
  
  /* 操作完成，停机 */
	while(1)
	{
	}
}

void LVGL_Test(){
	lv_obj_t * btn = lv_btn_create(lv_scr_act());
	lv_obj_t * label = lv_label_create(btn);
	lv_label_set_text(label,"hello");
	lv_obj_center(btn);
}

int main(){
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
	USART_Config();
	printf("窗口初始化完毕..\n");
	lv_init();
	lv_port_disp_init();
	lv_port_indev_init();
	LVGL_Test();
	printf("屏幕初始化完毕..\n");
	SysTick_Init();
	xTaskCreate(StartUpTask,"StartUpTask",StartUpTask_STACKSIZE,NULL,StartUpTask_PRIO,&StartUpTask_Handle);
	xTaskCreate(LvglTask,"LvglTask",LvglTask_STACKSIZE,NULL,LvglTask_PRIO,&LvglTask_Handle);
	xTaskCreate(FatFsTask,"FatFsTask",FatFsTask_STACKSIZE,NULL,FatFsTask_PRIO,&FATFStask_Handle);
	
	vTaskStartScheduler();
	
}
