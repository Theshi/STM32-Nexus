/**
 * @file    flash_resource.c
 * @brief   从外部 W25Q64 读资源索引表并按名字定位资源
 *          期望值数据来自烧录工程 burn_data[] 与 2026-08-09 实跑烧录日志（已逐字节校验）
 */
#include "flash_resource.h"
#include "bsp_spi_flash.h"

#include "lvgl.h"
#include "lv_port_disp_template.h"
#include "lv_port_indev_template.h"

#include <stdio.h>

/* 索引表读回后的 RAM 副本（672B） */
static Res_Index_Group res_hdr[RES_AREA_NUM];//一级索引副本
static Res_Index_Enter res_ent[RES_TOTAL_FILES];//二级索引副本
static uint8_t res_loaded = 0;

#define TEST_CODE 0//测试程序

/* ---------------- 测试数据的正确---------------- */

static const Res_Index_Group exp_hdr[RES_AREA_NUM] = {
    { 0xAABBCCDD, 1, 20, 0x300000, 0,    512 * 1024 },  /* 图标区 */
    { 0xAABBCCDD, 1,  1, 0x002000, 480, 2048 * 1024 },  /* 中文字库区 */
    { 0xAABBCCDD, 1,  4, 0x08D000, 504, 1060 * 1024 },  /* 英文字库区 */
};

static const Res_Index_Enter exp_ent[RES_TOTAL_FILES] = {
    /* ---- 图标区 20 个，addr = 0x300000 + 0x2000*i ---- */
    { "alarm",      0x300000, 5004 },
    { "refresh",    0x302000, 1804 },
    { "battery",    0x304000,  804 },
    { "bluetooth",  0x306000,  804 },
    { "left",       0x308000, 7204 },
    { "right",      0x30A000, 7204 },
    { "code",       0x30C000, 1254 },
    { "desktop",    0x30E000, 2454 },
    { "fan",        0x310000, 5004 },
    { "folder",     0x312000, 5004 },
    { "gamep",      0x314000, 5004 },
    { "set",        0x316000, 5004 },
    { "lightbulb",  0x318000, 5004 },
    { "paperplane", 0x31A000, 5004 },
    { "music",      0x31C000, 5004 },
    { "play",       0x31E000, 1804 },
    { "power",      0x320000, 1804 },
    { "wifi",       0x322000,  804 },
    { "menu",       0x324000, 6054 },
    { "terminal",   0x326000, 1254 },
    /* ---- 中文字库 1 个 ---- */
    { "sch16",      0x002000, 565991 },
    /* ---- 英文字库 4 个 ---- */
    { "eng_bt16",   0x08D000, 257018 },
    { "eng_ui16",   0x0CC000, 255547 },
    { "eng_ui28",   0x10B000, 274241 },
    { "eng_ui36",   0x14E000, 292486 },
};

/* ---------------- API ---------------- */

uint8_t Flash_Resource_Init(void)
{
    SPI_FLASH_BufferRead((uint8_t *)res_hdr, RES_INDEX_BASE, sizeof(res_hdr));
    SPI_FLASH_BufferRead((uint8_t *)res_ent, RES_INDEX_BASE + sizeof(res_hdr), sizeof(res_ent));
    res_loaded = 1;
    return 0;
}

uint8_t Flash_FindResource(const char *name, uint32_t *addr, uint32_t *size)
{
    uint8_t i;
    if (!res_loaded) return 0;
    for (i = 0; i < RES_TOTAL_FILES; i++) {
        if (strcmp(res_ent[i].name, name) == 0) {
            if (addr) *addr = res_ent[i].addr;
            if (size) *size = res_ent[i].size;
            return 1;
        }
    }
    return 0;
}

/* ---------------- LVGL调用显示外部图标 ---------------- */
/* 图标显示测试：从 Flash 读图标 bin（4B 头 + RGB565 像素）并显示
 * icon_buf/dsc 必须 static：lv_img_set_src 引用结构体与数据指针，不会拷贝 */
lv_obj_t *ui_icon(const char*name,//图标的名称
                 lv_obj_t *parent,//挂载的父对象
                 lv_align_t align,//图标的对齐方式
                 lv_coord_t x,//图标的x坐标
                 lv_coord_t y//图标的y坐标
)
{
    
	static lv_img_dsc_t dsc;
	uint32_t addr, size, hdr, cf, w, h;

	if(!Flash_FindResource(name, &addr, &size)){
		printf("icon [%s] not found!\r\n", name);
		return NULL;
	}

    uint8_t *icon_buf=lv_mem_alloc(size);          /* 最大图标 60×60×2+4 = 7204B */
    if(!icon_buf) return NULL;

	printf("read icon [%s]: addr=0x%06X size=%d\r\n", name, addr, size);
	SPI_FLASH_BufferRead(icon_buf, addr, size);

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

	lv_obj_t * img = lv_img_create(parent);
	lv_img_set_src(img, &dsc);
	/* 移到右上角，避免挡住下面的字库显示测试 */
	lv_obj_align(img, align, x, y);

    return img;
}

//调用外部字库的API封装
/* 字库显示测试：中英文都从 W25Q64 实时读字模（__user_font_getdata -> flash_font_getdata）。
 * 注意：烧录的字库不含空格(0x20)，英文空格由 fallback（montserrat）提供；
 *      中文测试串选用字库内含的字（"你好世界" 不在烧录的字符集里）。 */

lv_obj_t *ui_font(lv_obj_t *parent,//挂载的父对象
                  const char *txt,//显示的文本
                  const lv_font_t *font,//字体
                  lv_align_t align,//对齐方式
                  lv_coord_t x,//x坐标
                  lv_coord_t y//y坐标
)
{   

		lv_obj_t * lb = lv_label_create(parent);
		lv_label_set_text(lb, txt);
		lv_obj_set_style_text_font(lb, font, 0);
		lv_obj_align(lb, align, x,y);
        return lb;
}

/* ---------------- 测试 ---------------- */
#if TEST_CODE == 1
uint8_t Flash_Index_Test(void)
{
    uint8_t i, k, fail = 0;

    printf("\r\n======== Flash Index Test ========\r\n");

    Flash_Resource_Init();

    /* 1. 打印读回的表（与烧录工程 burn_index_table 打印格式一致） */
    printf("---- read back from flash 0x%06X ----\r\n", (unsigned int)RES_INDEX_BASE);
    for (i = 0; i < RES_AREA_NUM; i++) {
        Res_Index_Enter *sub = &res_ent[res_hdr[i].index_offset / sizeof(Res_Index_Enter)];
        printf("Area[%d]: magic=0x%08X ver=%d num=%d addr=0x%06X total=%dKB\r\n",
               i, res_hdr[i].magic, res_hdr[i].version, res_hdr[i].entry_num,
               res_hdr[i].area_addr, res_hdr[i].total_size / 1024);
        for (k = 0; k < res_hdr[i].entry_num; k++) {
            printf("  [%2d] %-16s addr=0x%06X size=%d\r\n", k, sub[k].name, sub[k].addr, sub[k].size);
        }
    }

    /* 2. 逐项与期望值比对 */
    printf("---- verify against expected ----\r\n");
    for (i = 0; i < RES_AREA_NUM; i++) {
        if (res_hdr[i].magic       != exp_hdr[i].magic ||
            res_hdr[i].version     != exp_hdr[i].version ||
            res_hdr[i].entry_num   != exp_hdr[i].entry_num ||
            res_hdr[i].area_addr   != exp_hdr[i].area_addr ||
            res_hdr[i].index_offset!= exp_hdr[i].index_offset ||
            res_hdr[i].total_size  != exp_hdr[i].total_size) {
            printf("  [FAIL] Area[%d] header mismatch\r\n", i);
            fail = 1;
        }
    }
    for (i = 0; i < RES_TOTAL_FILES; i++) {
        if (strcmp(res_ent[i].name, exp_ent[i].name) != 0 ||
            res_ent[i].addr != exp_ent[i].addr ||
            res_ent[i].size != exp_ent[i].size) {
            printf("  [FAIL] entry[%d] %s: expect addr=0x%06X size=%d got addr=0x%06X size=%d\r\n",
                   i, exp_ent[i].name, exp_ent[i].addr, exp_ent[i].size,
                   res_ent[i].addr, res_ent[i].size);
            fail = 1;
        }
    }

    if (fail == 0) {
        printf("  RESULT: ALL 25 ENTRIES MATCH OK\r\n");
    } else {
        printf("  RESULT: INDEX MISMATCH !!!\r\n");
    }
    printf("======== Flash Index Test END ========\r\n");
    return fail;
}

/* 诊断：dump 前 16 字节，并统计 check_len 范围内非 0xFF 的字节数（0xFF = 擦除态） */
void Flash_AreaDump(uint32_t addr, uint32_t check_len)
{
    uint8_t buf[256];
    uint16_t i;
    uint32_t nonff = 0, j;
    uint16_t step = 256;

    SPI_FLASH_BufferRead(buf, addr, 16);
    printf("dump 0x%06X: ", addr);
    for (i = 0; i < 16; i++) printf("%02X ", buf[i]);
    printf("\r\n");

    /* 统计非 0xFF 字节数，按 256B 分块读入 buf[256] */
    for (j = 0; j < check_len; j += step) {
        uint16_t n = (check_len - j > step) ? step : (uint16_t)(check_len - j);
        uint16_t k;
        SPI_FLASH_BufferRead(buf, addr + j, n);
        for (k = 0; k < n; k++) {
            if (buf[k] != 0xFF) nonff++;
        }
    }
    printf("  non-0xFF bytes in %d byte window @0x%06X: %d\r\n",
           check_len, addr, nonff);
}
#endif