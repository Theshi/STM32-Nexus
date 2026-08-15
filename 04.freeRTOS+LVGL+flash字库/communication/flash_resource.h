#ifndef __FLASH_RESOURCE_H
#define __FLASH_RESOURCE_H

#include "stm32f10x.h"
#include <string.h>

#include "lvgl.h"

/* 资源区域数 / 总文件数（与烧录工程 aux_data.h 一致） */
#define RES_AREA_NUM       3
#define RES_TOTAL_FILES    25

/* 索引表在 Flash 的基址（0x000000，一级 72B + 二级 600B = 672B） */
#define RES_INDEX_BASE     0x000000

/* 一级索引：每个区域一条 */
typedef struct {
    uint32_t magic;          /* 0xAABBCCDD */
    uint32_t version;        /* 1 */
    uint32_t entry_num;      /* 该区文件数：20 / 1 / 4 */
    uint32_t area_addr;      /* 区起始地址 */
    uint32_t index_offset;   /* 该区 index_enter 子表相对 index_enter[0] 的字节偏移 */
    uint32_t total_size;     /* 区预留总大小 */
} Res_Index_Group;

/* 二级索引：每个文件一条 */
typedef struct {
    char     name[16];       /* 资源名，与 .bin 同名 */
    uint32_t addr;           /* 该文件在 Flash 的地址 */
    uint32_t size;           /* 该文件实际字节数 */
} Res_Index_Enter;

/* 初始化：从 Flash 0x000000 读索引表进 RAM（672B），返回 0 成功 */
uint8_t Flash_Resource_Init(void);

lv_obj_t *ui_icon(const char*name,//图标的名称
                 lv_obj_t *parent,//挂载的父对象
                 lv_align_t align,//图标的对齐方式
                 lv_coord_t x,//图标的x坐标
                 lv_coord_t y//图标的y坐标
);
lv_obj_t *ui_font(lv_obj_t *parent,//挂载的父对象
                  const char *txt,//显示的文本
                  const lv_font_t *font,//字体
                  lv_align_t align,//对齐方式
                  lv_coord_t x,//x坐标
                  lv_coord_t y//y坐标
);
/* 按名字查资源，找到返回 1 并输出 addr/size；未找到返回 0 */
uint8_t Flash_FindResource(const char *name, uint32_t *addr, uint32_t *size);

/* 测试：读索引表、打印并逐项比对期望值，全部一致返回 0 */
uint8_t Flash_Index_Test(void);

/* 诊断：dump 几个关键地址的前 16 字节，并在指定范围统计非 0xFF 字节数 */
void Flash_AreaDump(uint32_t addr, uint32_t check_len);

#endif /* __FLASH_RESOURCE_H */
