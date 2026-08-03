/**
  ******************************************************************************
  * @file    aux_data.c
  * @author  fire
  * @version V1.0
  * @date    2015-xx-xx
  * @brief   Burn data from SD card to FLASH
  ******************************************************************************
  * @attention
  *
  * Platform: Wildfire STM32 Guide Development Board
  * Forum    :http://www.firebbs.cn
  * Taobao   :https://fire-stm32.taobao.com
  *
  ******************************************************************************
  */

#include "aux_data.h"
#include "ff.h"
#include "bsp_spi_flash.h"
#include "bsp_led.h"

/***************************************************************************************************************/

//SD card source data path
char src_dir[255]="0:/srcdata";

//FLASH target path
char dst_dir[255]= FLASH_ROOT;

char flash_scan_dir[255]= FLASH_ROOT;
char sd_scan_dir[255]= SD_ROOT;

/* ======== level-2 index table: one entry per FILE, filled by burn loop ======== */
Index_Enter index_enter[AUX_TOTAL_FILES];

/* ======== level-1 index: one entry per AREA ======== */
Index_Group index_header[AUX_MAX_NUM]=
{
  [AUX_ICON_BIN]=
  {
    .magic=0xAABBCCDD,
    .version=0x0001,
    .entry_num=AUX_ICON_NUM,
    .area_addr=0x300000,
    .total_size=512*1024,
    .index_offset=0,
  },
  [AUX_CH_FONT_BIN]=
  {
    .magic=0xAABBCCDD,
    .version=0x0001,
    .entry_num=AUX_CH_FONT_NUM,
    .area_addr=0x002000,
    .total_size=2*1024*1024,
    .index_offset=AUX_ICON_NUM*sizeof(Index_Enter),
  },
  [AUX_ENG_FONT_BIN]=
  {
    .magic=0xAABBCCDD,
    .version=0x0001,
    .entry_num=AUX_EN_FONT_NUM,
    .area_addr=0x008D000,
    .total_size=(63+63+67+72)*4096,
    .index_offset=(AUX_ICON_NUM+AUX_CH_FONT_NUM)*sizeof(Index_Enter),
  }
};

/* ======== file description: one entry per FILE to burn ======== */
Aux_Data_Typedef burn_data[AUX_TOTAL_FILES] =
{
  /* ---- icon area: 20 icons @ 0x300000, 8KB each ---- */
  { "alarm",     "0:/icon_bin/alarm.bin",     "icon alarm",     0x300000, 2*4096, UPDATE },
  { "refresh",   "0:/icon_bin/refresh.bin",   "icon refresh",   0x302000, 2*4096, UPDATE },
  { "battery",   "0:/icon_bin/battery.bin",   "icon battery",   0x304000, 2*4096, UPDATE },
  { "bluetooth", "0:/icon_bin/bluetooth.bin", "icon bluetooth", 0x306000, 2*4096, UPDATE },
  { "left",      "0:/icon_bin/left.bin",      "icon left",      0x308000, 2*4096, UPDATE },
  { "right",     "0:/icon_bin/right.bin",     "icon right",     0x30A000, 2*4096, UPDATE },
  { "code",      "0:/icon_bin/code.bin",      "icon code",      0x30C000, 2*4096, UPDATE },
  { "desktop",   "0:/icon_bin/desktop.bin",   "icon desktop",   0x30E000, 2*4096, UPDATE },
  { "fan",       "0:/icon_bin/fan.bin",       "icon fan",       0x310000, 2*4096, UPDATE },
  { "folder",    "0:/icon_bin/folder.bin",    "icon folder",    0x312000, 2*4096, UPDATE },
  { "game",      "0:/icon_bin/game.bin",      "icon game",      0x314000, 2*4096, UPDATE },
  { "setting",   "0:/icon_bin/setting.bin",   "icon setting",   0x316000, 2*4096, UPDATE },
  { "lightbulb", "0:/icon_bin/lightbulb.bin", "icon lightbulb", 0x318000, 2*4096, UPDATE },
  { "plane",     "0:/icon_bin/plane.bin",     "icon plane",     0x31A000, 2*4096, UPDATE },
  { "music",     "0:/icon_bin/music.bin",     "icon music",     0x31C000, 2*4096, UPDATE },
  { "play",      "0:/icon_bin/play.bin",      "icon play",      0x31E000, 2*4096, UPDATE },
  { "power",     "0:/icon_bin/power.bin",     "icon power",     0x320000, 2*4096, UPDATE },
  { "wifi",      "0:/icon_bin/wifi.bin",      "icon wifi",      0x322000, 2*4096, UPDATE },
  { "menu",      "0:/icon_bin/menu.bin",      "icon menu",      0x324000, 2*4096, UPDATE },
  { "terminal",  "0:/icon_bin/terminal.bin",  "icon terminal",  0x326000, 2*4096, UPDATE },

  /* ---- chinese font ---- */
  { "sch16",     "0:/ch_font/my_font_SCH_16.bin",  "chinese font 16",  0x002000, 139*4096, UPDATE },

  /* ---- english fonts ---- */
  { "eng_bt16",  "0:/en_font/my_font_ENG_BT_16.bin", "english BT 16", 0x008D000, 63*4096, UPDATE },
  { "eng_ui16",  "0:/en_font/my_font_ENG_UI_16.bin", "english UI 16", 0x00CC000, 63*4096, UPDATE },
  { "eng_ui28",  "0:/en_font/my_font_ENG_UI_28.bin", "english UI 28", 0x010B000, 67*4096, UPDATE },
  { "eng_ui36",  "0:/en_font/my_font_ENG_UI_36.bin", "english UI 36", 0x014E000, 72*4096, UPDATE },
};



static int copy_dir(char *src_path,char *dst_path);
static int copy_file(char *src_path,char *dst_path);
static FRESULT scan_files (char* path) ;
static FRESULT copy_all (char* src_path,char* dst_path);

/* file system objects for SD card and FLASH */
FATFS flash_fs;
FATFS sd_fs;													/* Work area (file system object) for logical drives */



/* file object used for SD access */
static FIL fnew;												/* file objects */

/**
  * @brief  burn files into FLASH (raw address area, no file system on FLASH)
  * @param  dat: burn info of the files to write
  * @param  file_num: number of files to burn
  * @retval FR_OK on success
  */
FRESULT burn_file_sd2flash(Aux_Data_Typedef *dat,uint8_t file_num)
{
    uint8_t i;

    FRESULT result;
    UINT  bw;            					    /* File R/W count */


    uint32_t write_addr=0,j=0;
    uint8_t tempbuf[256],flash_buf[256];

//    result = f_mount(&sd_fs,SD_ROOT,1);
//
//    //quit if SD file system mount failed
//    if(result != FR_OK)
//    {
//      BURN_ERROR("f_mount ERROR!");
//      LED_RED;
//      return result;
//    }

    for(i=0;i<file_num;i++)
    {
       if (dat[i].burn_option == DO_NOT_UPDATE)
          continue;

       BURN_INFO("-------------------------------------");
       BURN_INFO("preparing to burn: %s",dat[i].description);
       LED_BLUE;

       result = f_open(&fnew,dat[i].filename,FA_OPEN_EXISTING | FA_READ);
        if(result != FR_OK)
        {
            BURN_ERROR("open file failed!");
            LED_RED;
            return result;
        }

      BURN_INFO("erasing FLASH area ...");

      write_addr = dat[i].start_addr;

      for(j=0;j < dat[i].length/4096 ;j++)//file size is a multiple of 4KB
      {
        SPI_FLASH_SectorErase(write_addr+j*4096);
      }

      BURN_INFO("writing to FLASH ...");

      write_addr = dat[i].start_addr;
      while(result == FR_OK)
      {
        result = f_read( &fnew, tempbuf, 256, &bw);//read data
        if(result!=FR_OK)			 //error
        {
          BURN_ERROR("read file failed!");
          LED_RED;
          return result;
        }
        SPI_FLASH_PageWrite(tempbuf,write_addr,256);  //write data to external flash
        write_addr+=256;
        if(bw !=256)break;
      }

        BURN_INFO("write done, verifying ...");

        //verify data
      write_addr = dat[i].start_addr;

      f_lseek(&fnew,0);

      while(result == FR_OK)
      {
        result = f_read( &fnew, tempbuf, 256, &bw);//read data
        if(result!=FR_OK)			 //error
        {
          BURN_ERROR("read file failed!");
          LED_RED;
          return result;
        }

        SPI_FLASH_BufferRead(flash_buf,write_addr,bw);  //read data from FLASH
        write_addr+=bw;

        for(j=0;j<bw;j++)
        {
          if(tempbuf[j] != flash_buf[j])
          {
            BURN_ERROR("verify failed!");
            LED_RED;
            return FR_INT_ERR;
          }
         }

        if(bw !=256)break;//file tail reached
      }


      BURN_INFO("verify OK!");
      BURN_INFO("file: %s burned",dat[i].filename);
      BURN_INFO("-------------------------------------");
      LED_BLUE;

      /* fill level-2 index entry for this file */
      memset(index_enter[i].name, 0, sizeof(index_enter[i].name));
      strncpy(index_enter[i].name, dat[i].name, sizeof(index_enter[i].name)-1);
      index_enter[i].addr = dat[i].start_addr;
      index_enter[i].size = f_size(&fnew);   /* actual file size in bytes */

      f_close(&fnew);
    }


    BURN_INFO("************************************");
    BURN_INFO("all files burned OK! (non-filesystem part)");
    return FR_OK;


}

/* Alternative approach: treat W25Q64 as a FAT drive, burn via a FATFS file system */

/**
  * @brief  copy files (recursively if there are subdirectories)
  * @param  src_path: file (or folder) to copy
  * @param  dst_path: target folder
  * @retval result: file system return value
  */
FRESULT copy_file_sd2flash(char *src_path,char *dst_path)
{
  FRESULT result;

  BURN_INFO("-------------------------------------");
  //copy files to the file system area of flash
  BURN_INFO("about to copy files to the FLASH file system area ...");


  //mount flash
  result = f_mount(&flash_fs,FLASH_ROOT,1);

  BURN_INFO("formatting FLASH ...");

  //format FLASH
  result = f_mkfs(FLASH_ROOT,0,0);

  //remount flash
  result = f_mount(NULL,FLASH_ROOT,1);
  result = f_mount(&flash_fs,FLASH_ROOT,1);

  //quit if file system mount failed
  if(result != FR_OK)
  {
    BURN_ERROR("FLASH file system mount failed, reset and retry!");
    LED_RED;
    return result;
  }

  BURN_INFO("*****************************************");
  BURN_INFO("\r\n SD card files to be copied: \r\n");
  scan_files(sd_scan_dir);

  BURN_INFO("*****************************************");

  BURN_INFO("\r\n start copying \r\n");
  result = copy_all(src_dir,dst_dir);
  if(result != FR_OK)
  {
    BURN_ERROR("copy to FLASH failed, code: %d",result);
    LED_RED;
    return result;
  }

  BURN_INFO("*****************************************");
  BURN_INFO("\r\n FLASH files after copy (file system part): \r\n");
  scan_files(flash_scan_dir);

  BURN_INFO("*****************************************");
  BURN_INFO("all files copied OK! (file system part)");
  LED_BLUE;

  return result;

}


/**
  * @brief  scan_files: recursively scan files in FatFs
  * @param  path: initial scan path
  * @retval result: file system return value
  */
static FRESULT scan_files (char* path)
{
  FRESULT res; 		//some variables are modified during recursion, so not global
  FILINFO fno;
  DIR dir;
  int i;
  char *fn;        // file name

#if _USE_LFN
  /* long file name support */
  /* simplified chinese needs 2 bytes per character */
  static char lfn[_MAX_LFN*2 + 1];
  fno.lfname = lfn;
  fno.lfsize = sizeof(lfn);
#endif
  //open directory
  res = f_opendir(&dir, path);
  if (res == FR_OK)
	{
    i = strlen(path);
    for (;;)
		{
      //read directory entry, next call reads the next one
      res = f_readdir(&dir, &fno);
      //empty means all entries read, break
      if (res != FR_OK || fno.fname[0] == 0) break;
#if _USE_LFN
      fn = *fno.lfname ? fno.lfname : fno.fname;
#else
      fn = fno.fname;
#endif
      //dot means current directory, skip
      if (*fn == '.') continue;
      //directory, recurse
      if (fno.fattrib & AM_DIR)
			{
        //build full directory name
        sprintf(&path[i], "/%s", fn);
        BURN_INFO("folder: %s",path);
        //recurse
        res = scan_files(path);
        path[i] = 0;
        //open failed, break
        if (res != FR_OK)
					break;
      }
			else
			{
				BURN_INFO("%s/%s", path, fn);								//print file name
        /* can extract specific-format file paths here */
      }//else
    } //for
  }
  return res;
}


#define COPY_UNIT 4096

static FIL fsrc,fdst;													/* file objects */
static BYTE read_buf[COPY_UNIT]={0};        /* read buffer */
static UINT real_read_num;            					  /* bytes actually read/written */
static UINT real_write_num;            					  /* bytes actually read/written */

char newfn[255];

/**
  * @brief  copy a file
  * @param  src_path: source file path
  * @param  dst_path: destination folder (without file name)
  * @retval result: file system return value
  */
static int copy_file(char *src_path,char *dst_path)
{
    FRESULT res = FR_OK;
    char *sub_dir = NULL;

    res = f_open(&fsrc,src_path,FA_READ);
    require_noerr(res,exit);

   //get sub file path
    sub_dir =  strrchr(src_path,'/');
    require_noerr(!sub_dir,exit);

    //build new path
    sprintf(newfn,"%s%s",dst_path,sub_dir);

    BURN_INFO("copying file %s ...",newfn);
    res = f_open(&fdst,newfn,FA_CREATE_ALWAYS|FA_WRITE|FA_READ);
    require_noerr(res,exit);

    //copy file
    while(res == FR_OK)
    {
      res = f_read(&fsrc,&read_buf,COPY_UNIT,&real_read_num);
      require_noerr(res,exit);

      res = f_write(&fdst,&read_buf,real_read_num,&real_write_num);
      require_noerr(res,exit);

      if(real_read_num != COPY_UNIT)
        break;
    }

    f_close(&fsrc);
    f_close(&fdst);

    BURN_INFO("copy OK");

exit:
    return res;
}


/**
  * @brief  create a folder
  * @param  src_path: source folder path
  * @param  dst_path: where to create the folder
  * @retval result: file system return value
  */
static int copy_dir(char *src_path,char *dst_path)
{
    FRESULT res;
    char *sub_dir = NULL;

    //get sub folder path
    sub_dir =  strrchr(src_path,'/');
    require_noerr(!sub_dir,exit);

    BURN_DEBUG("path to create: %s",dst_path);

    //create path
    res = f_mkdir(dst_path);

    //folder already exists, fine
    if(res == FR_EXIST)
      res = FR_OK;

    //check
    require_noerr(res,exit);

exit:
    return res;
}


//cache for source file name
static char fntemp[_MAX_LFN*2 + 1];

/**
  * @brief  copy files (recursively if there are subdirectories)
  * @param  src_path: file (or folder) to copy
  * @param  dst_path: target folder
  * @retval result: file system return value
  */
static FRESULT copy_all (char* src_path,char* dst_path)
{
  FRESULT res; 		//some variables are modified during recursion, so not global
  FILINFO fno;
  DIR dir;
  int i,j;
  char *fn;        // file name

#if _USE_LFN
  /* long file name support */
  /* simplified chinese needs 2 bytes per character */
  static char lfn[_MAX_LFN*2 + 1];
  fno.lfname = lfn;
  fno.lfsize = sizeof(lfn);
#endif

  //open directory
  res = f_opendir(&dir, src_path);
  if (res == FR_OK)
	{
    i = strlen(src_path);//source path
    j = strlen(dst_path);//target path
    for (;;)
		{
      //read directory entry, next call reads the next one
      res = f_readdir(&dir, &fno);
      //empty means all entries read, break
      if (res != FR_OK || fno.fname[0] == 0) break;
#if _USE_LFN
      fn = *fno.lfname ? fno.lfname : fno.fname;
#else
      fn = fno.fname;
#endif
      //dot means current directory, skip
      if (*fn == '.') continue;
      //directory, recurse
      if (fno.fattrib & AM_DIR)
			{
        //build full directory name
        sprintf(&src_path[i], "/%s", fn);
        BURN_DEBUG("src dir=%s",src_path);

        sprintf(&dst_path[j], "/%s", fn);
        BURN_DEBUG("dst dir=%s",dst_path);
        copy_dir(src_path,dst_path);

        //recurse
        res = copy_all(src_path,dst_path);
        src_path[i] = 0;
        dst_path[j] = 0;
        //open failed, break
        if (res != FR_OK)
					break;
      }
			else
			{
				BURN_DEBUG("%s/%s", src_path, fn);								//print file name
        sprintf(fntemp,"%s/%s",src_path,fn);
        BURN_DEBUG("%s",fntemp);
        BURN_DEBUG("dst_path = %s",dst_path);
        /* can extract specific-format file paths here */
        copy_file(fntemp,dst_path);
      }//else
    } //for
  }
  return res;
}




/**
  * @brief  write level-1/level-2 index table to flash 0x000000 (resource table area)
  *         then read back and compare to verify burn success.
  * @retval FR_OK on success, else FR_INT_ERR
  */
FRESULT burn_index_table(void)
{
    Index_Group  hdr[AUX_MAX_NUM];
    Index_Enter  ent[AUX_TOTAL_FILES];
    uint32_t     i, k;

    BURN_INFO("-------------------------------------");
    BURN_INFO("write index table to 0x000000 ...");

    /* erase resource table sector (4KB, table is only 672 bytes) */
    SPI_FLASH_SectorErase(0x000000);

    /* write level-1: index_header */
    SPI_FLASH_BufferWrite((u8*)index_header, 0x000000, sizeof(index_header));

    /* write level-2: index_enter */
    SPI_FLASH_BufferWrite((u8*)index_enter, sizeof(index_header), sizeof(index_enter));

    /* read back and compare to verify */
    SPI_FLASH_BufferRead((u8*)hdr, 0x000000, sizeof(hdr));
    SPI_FLASH_BufferRead((u8*)ent, sizeof(index_header), sizeof(ent));

    if(memcmp(hdr, index_header, sizeof(hdr)) != 0 ||
       memcmp(ent, index_enter, sizeof(ent)) != 0)
    {
        BURN_ERROR("index table verify FAILED");
        LED_RED;
        return FR_INT_ERR;
    }

    /* print the index table for eyeball check */
    BURN_INFO("----- index table -----");
    for(i = 0; i < AUX_MAX_NUM; i++)
    {
        Index_Enter *sub = &ent[hdr[i].index_offset / sizeof(Index_Enter)];
        BURN_INFO("Area[%d]: magic=0x%08X version=%d entry_num=%d addr=0x%06X total=%dKB",
                  i, hdr[i].magic, hdr[i].version, hdr[i].entry_num,
                  hdr[i].area_addr, hdr[i].total_size/1024);
        for(k = 0; k < hdr[i].entry_num; k++)
        {
            BURN_INFO("  [%d] %-16s addr=0x%06X size=%d", k, sub[k].name, sub[k].addr, sub[k].size);
        }
    }

    BURN_INFO("index table verify OK");
    return FR_OK;
}


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
