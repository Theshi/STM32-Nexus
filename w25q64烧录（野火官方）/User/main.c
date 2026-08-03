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
  ******************************************************************************
  */

#include "stm32f10x.h"
#include "./usart/bsp_usart.h"
#include "./flash/bsp_spi_flash.h"
#include "./FATFS/ff.h"
#include "./FATFS/diskio.h"
#include "./led/bsp_led.h"
#include "aux_data.h"

/* FatFS work area (SD card = drive 0) */
static FATFS sd_fs;

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

    /* 2. mount SD card */
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

    /* 3. burn all files in burn_data[] (each file is byte-verified) */
    printf("\r\nBurning %d files ...\r\n", AUX_TOTAL_FILES);
    res = burn_file_sd2flash(burn_data, AUX_TOTAL_FILES);
    if (res != FR_OK)
    {
        printf("ERROR: burn failed! code=%d\r\n", res);
        LED_RED;
        while (1);
    }
    printf("All files burned OK!\r\n");

    /* 4. write index table to flash 0x000000 and verify */
    res = burn_index_table();
    if (res != FR_OK)
    {
        printf("ERROR: index table write/verify failed! code=%d\r\n", res);
        LED_RED;
        while (1);
    }

    /* 5. done */
    printf("\r\n============================================\r\n");
    printf("   BURN COMPLETE!                           \r\n");
    printf("============================================\r\n");
    LED_GREEN;

    while (1);
}
