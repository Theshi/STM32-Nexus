/**
  ******************************************************************************
  * @file    main.c
  * @brief   Burn LVGL fonts/icons from SD card to W25Q64
  *          Based on Wildfire's aux_data.c burn framework
  ******************************************************************************
  * Usage:
  *   1. Put .bin files on SD card at paths listed in burn_data[] (aux_data.c)
  *   2. Insert SD card, power on, burn starts automatically
  *   3. Watch UART output (115200 8N1)
  *
  * Idempotent guard (cheap, no SD needed):
  *   On startup a fast index check decides whether flash looks burned already.
  *   If it does, the burn is skipped so a reset / power-on / interrupted
  *   download that re-runs main() will NOT erase a good flash.
  *   To re-burn new content (e.g. regenerated icons): press KEY2 (PC13).
  *   Hold KEY2 at startup to force a re-burn immediately.
  ******************************************************************************
  */

#include "stm32f10x.h"
#include <string.h>
#include "./usart/bsp_usart.h"
#include "./flash/bsp_spi_flash.h"
#include "./FATFS/ff.h"
#include "./FATFS/diskio.h"
#include "./led/bsp_led.h"
#include "./key/bsp_key.h"
#include "aux_data.h"

/* FatFS work area (SD card = drive 0) */
static FATFS sd_fs;

/* Returns 1 if the flash index is valid and every resource slot has real
 * (non-blank) content.  This is a CHEAP read-only check (no SD access), used
 * to keep a reset / power-on / interrupted download from erasing a good
 * flash.  It does NOT compare file contents, so re-burning new content is
 * done explicitly by holding/pressing KEY2. */
static int flash_already_burned(void)
{
    Index_Group  hdr[AUX_MAX_NUM];
    Index_Enter  ent[AUX_TOTAL_FILES];
    uint8_t i, k, buf[16];
    int blank;

    SPI_FLASH_BufferRead((u8 *)hdr, 0x000000, sizeof(hdr));
    SPI_FLASH_BufferRead((u8 *)ent, sizeof(hdr), sizeof(ent));

    if (hdr[0].magic != 0xAABBCCDD || hdr[0].version != 1) return 0;

    for (i = 0; i < AUX_TOTAL_FILES; i++) {
        if (strncmp(ent[i].name, burn_data[i].name, sizeof(ent[i].name)) != 0) return 0;
        if (ent[i].addr != burn_data[i].start_addr) return 0;
        /* slot must not be blank (0xFF) - catches interrupted/erased files */
        SPI_FLASH_BufferRead(buf, ent[i].addr, sizeof(buf));
        blank = 1;
        for (k = 0; k < sizeof(buf); k++) {
            if (buf[k] != 0xFF) { blank = 0; break; }
        }
        if (blank) return 0;
    }
    return 1;
}

/* crude blocking delay used to debounce the KEY2 poll loop */
static void delay_loop(volatile uint32_t n)
{
    while (n--) {
        volatile uint32_t j;
        for (j = 0; j < 10000; j++);
    }
}

static uint8_t key2_pressed(void)
{
    return GPIO_ReadInputDataBit(KEY2_GPIO_PORT, KEY2_GPIO_PIN) == KEY_ON;
}

/* Debounced KEY2 read: the pin must stay pressed for a few ms, so contact
 * bounce at power-on / during a press cannot trigger a spurious re-burn. */
static uint8_t key2_debounced(void)
{
    if (!key2_pressed()) return 0;
    delay_loop(5);                 /* ~2ms settle time */
    return key2_pressed();
}

int main(void)
{
    FRESULT res;
    uint32_t flash_id;

    /* init peripherals */
    USART_Config();
    LED_GPIO_Config();

    printf("\r\n============================================\r\n");
    printf("   W25Q64 Burn Tool (SD -> Flash)          \r\n");
    printf("============================================\r\n");

    /* 1. check SPI flash */
    SPI_FLASH_Init();
    flash_id = SPI_FLASH_ReadID();
    printf("W25Q64 ID: 0x%06X\r\n", flash_id);

    if (flash_id != 0xEF4017)
    {
        printf("ERROR: W25Q64 not found (expect 0xEF4017)!\r\n");
        LED_RED;
        while (1);
    }
    printf("W25Q64 OK! (8MB)\r\n");

    /* 2. idempotency guard: skip only if flash already looks burned.
     *    Hold KEY2 (PC13) at startup, or press it after the message below,
     *    to force a re-burn with the SD card content. */
    Key_GPIO_Config();
    if (key2_debounced()) {
        printf("KEY2 pressed at startup: forcing re-burn.\r\n");
    } else if (flash_already_burned()) {
        printf("\r\nFlash already burned (index valid, all slots have data).\r\n");
        printf("Press KEY2 (PC13) now to re-burn from the SD card,\r\n");
        printf("or reset to keep the current flash content.\r\n");
        LED_GREEN;
        /* poll KEY2 continuously: any debounced press triggers a re-burn */
        for (;;) {
            delay_loop(5);
            if (key2_debounced()) {
                printf("KEY2 pressed: re-burning.\r\n");
                break;
            }
        }
    }

    /* 3. mount SD card */
    printf("\r\nMounting SD card ...\r\n");
    res = f_mount(&sd_fs, "0:", 1);
    if (res != FR_OK)
    {
        printf("ERROR: SD mount failed! code=%d\r\n", res);
        printf("Check: SD inserted? FAT32 formatted?\r\n");
        LED_RED;
        while (1);
    }
    printf("SD card mounted OK!\r\n");

    /* 4. burn all files in burn_data[] (each file is byte-verified) */
    printf("\r\nBurning %d files ...\r\n", AUX_TOTAL_FILES);
    res = burn_file_sd2flash(burn_data, AUX_TOTAL_FILES);
    if (res != FR_OK)
    {
        printf("ERROR: burn failed! code=%d\r\n", res);
        LED_RED;
        while (1);
    }
    printf("All files burned OK!\r\n");

    /* 5. write index table to flash 0x000000 and verify */
    res = burn_index_table();
    if (res != FR_OK)
    {
        printf("ERROR: index table write/verify failed! code=%d\r\n", res);
        LED_RED;
        while (1);
    }

    /* 6. done */
    printf("\r\n============================================\r\n");
    printf("   BURN COMPLETE!                           \r\n");
    printf("============================================\r\n");
    LED_GREEN;

    while (1);
}
