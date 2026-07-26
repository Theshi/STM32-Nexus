/*-----------------------------------------------------------------------*/
/* Low level disk I/O module for FatFs     (C)ChaN, 2019                */
/*-----------------------------------------------------------------------*/
/* 功能：FatFs 与硬件之间的桥梁层                                          */
/* 本文件实现 disk_status / disk_initialize / disk_read / disk_write /   */
/* disk_ioctl / get_fattime 共 6 个接口函数                               */
/* 当前配置：仅支持 SD 卡（SDIO 接口），物理驱动器 0 号                    */
/*-----------------------------------------------------------------------*/
#include <string.h>
#include "stm32f10x.h"
#include "ff.h"			/* 提供整数类型定义（BYTE、DWORD、LBA_t 等） */
#include "diskio.h"		/* 声明 disk_xxx 系列函数原型 */

#include "bsp_sdio_sdcard.h"

/*=========================================================================*/
/*                      物理驱动器编号定义                                  */
/*=========================================================================*/
/* 每个编号对应一个物理存储设备，FatFs 通过 pdrv 参数选择操作哪个驱动器      */
/* 当前只使用 SD 卡，未来扩展外部 Flash 时添加 SPI_FLASH = 1 即可           */
#define SD_CARD		0	/* SD卡 -> 物理驱动器 0 */
/* #define SPI_FLASH	1 */	/* 预留：外部 SPI Flash -> 物理驱动器 1 */

/* SD 卡扇区大小（单位：字节），SDSC/SDHC 固定为 512 */
#define SD_BLOCKSIZE   512

/* SD 卡信息结构体，由 bsp_sdio_sdcard.c 中的 SD_Init() 填充 */
extern SD_CardInfo   SDCardInfo;


/*=========================================================================*/
/*  获取驱动器状态                                                         */
/*  参数：pdrv - 物理驱动器编号                                            */
/*  返回：DSTATUS - 0=正常，STA_NOINIT=未初始化，STA_NODISK=无介质          */
/*=========================================================================*/
DSTATUS disk_status (
	BYTE pdrv		/* Physical drive number to identify the drive */
)
{
	DSTATUS stat = 0;	/* 务必初始化为 0，否则 &= 运算结果不确定 */

	switch (pdrv) {
	case SD_CARD:
		/* 标记介质存在（不检测卡是否在位，如有检测引脚可扩展） */
		stat &= ~STA_NODISK;
		return stat;

	default:
		return STA_NOINIT;	/* 未知驱动器返回未初始化 */
	}
}


/*=========================================================================*/
/*  初始化驱动器                                                           */
/*  参数：pdrv - 物理驱动器编号                                            */
/*  返回：DSTATUS - 0=成功，STA_NOINIT=初始化失败                           */
/*  注意：SD_Init() 内部会完成 SDIO 外设配置、卡识别、时钟切换、4bit 总线   */
/*=========================================================================*/
DSTATUS disk_initialize (
	BYTE pdrv		/* Physical drive number to identify the drive */
)
{
	DSTATUS stat = 0;

	switch (pdrv) {
	case SD_CARD:
		if (SD_Init() == SD_OK) {
			/* 初始化成功，清除无介质标志 */
			stat &= ~STA_NODISK;
		} else {
			/* 初始化失败（卡未插入、通信异常等） */
			stat = STA_NOINIT;
		}
		return stat;

	default:
		return STA_NOINIT;
	}
}


/*=========================================================================*/
/*  读取扇区                                                               */
/*  参数：                                                                 */
/*    pdrv   - 物理驱动器编号                                              */
/*    buff   - 数据缓冲区指针（需注意 4 字节对齐要求）                      */
/*    sector - 起始 LBA 扇区号                                             */
/*    count  - 需要读取的扇区数量                                          */
/*  返回：DRESULT - RES_OK=成功，RES_PARERR=参数错误，RES_ERROR=硬件错误    */
/*=========================================================================*/
DRESULT disk_read (
	BYTE pdrv,		/* Physical drive number to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	DRESULT stat = RES_PARERR;
	SD_Error SD_state = SD_OK;

	switch (pdrv) {
	case SD_CARD:
		/*=================================================================*/
		/* 关键！SDIO DMA 要求缓冲区 4 字节对齐                             */
		/* 如果传入的 buff 地址未对齐（低 2 位非 0），不能直接传给 DMA      */
		/* 处理方式：通过栈上的对齐临时缓冲区逐扇区读取，再 memcpy 到目标   */
		/*=================================================================*/
		if ((DWORD)buff & 3) {
			DRESULT res = RES_OK;
			/* 栈上分配 512 字节的 4 字节对齐缓冲区（DWORD 数组天然对齐） */
			DWORD scratch[SD_BLOCKSIZE / 4];

			while (count--) {
				/* 递归调用自身，此时 scratch 是对齐的，走下方 DMA 分支 */
				res = disk_read(SD_CARD, (void *)scratch, sector++, 1);
				if (res != RES_OK) {
					break;
				}
				/* 将对齐缓冲区中的数据拷贝到用户的不对齐缓冲区 */
				memcpy(buff, scratch, SD_BLOCKSIZE);
				buff += SD_BLOCKSIZE;
			}
			return res;
		}

		/*=============================================================*/
		/* 缓冲区已对齐，使用 SDIO DMA 多块读取                        */
		/* SD_ReadMultiBlocks 参数：                                   */
		/*   buff    - 数据缓冲区                                      */
		/*   地址    = sector * 扇区大小（字节地址，非 LBA）            */
		/*   SD_BLOCKSIZE - 块大小（固定 512）                          */
		/*   count   - 块数量                                          */
		/*=============================================================*/
		SD_state = SD_ReadMultiBlocks(buff, (uint64_t)sector * SD_BLOCKSIZE,
									   SD_BLOCKSIZE, count);
		if (SD_state == SD_OK) {
			/* 等待 DMA 传输完成 */
			SD_state = SD_WaitReadOperation();
			/* 等待 SDIO 控制器状态机回到就绪 */
			while (SD_GetStatus() != SD_TRANSFER_OK);
		}
		if (SD_state != SD_OK)
			stat = RES_PARERR;
		else
			stat = RES_OK;
		break;

	default:
		stat = RES_PARERR;
		break;
	}
	return stat;
}


/*=========================================================================*/
/*  写入扇区（仅在 FF_FS_READONLY == 0 时编译）                            */
/*  参数：                                                               */
/*    pdrv   - 物理驱动器编号                                            */
/*    buff   - 待写入数据缓冲区指针                                      */
/*    sector - 起始 LBA 扇区号                                           */
/*    count  - 需要写入的扇区数量                                        */
/*  返回：DRESULT - RES_OK=成功，RES_PARERR=参数错误，RES_ERROR=硬件错误  */
/*=========================================================================*/
#if FF_FS_READONLY == 0

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive number to identify the drive */
	const BYTE *buff,	/* Data to be written */
	LBA_t sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	DRESULT stat = RES_PARERR;
	SD_Error SD_state = SD_OK;

	/* 参数检查：写入 0 个扇区是非法的 */
	if (!count) {
		return RES_PARERR;
	}

	switch (pdrv) {
	case SD_CARD:
		/*=============================================================*/
		/* 与读操作相同，缓冲区未对齐时通过临时缓冲逐扇区写入            */
		/*=============================================================*/
		if ((DWORD)buff & 3) {
			DRESULT res = RES_OK;
			DWORD scratch[SD_BLOCKSIZE / 4];

			while (count--) {
				/* 先将用户数据拷贝到对齐的临时缓冲区 */
				memcpy(scratch, buff, SD_BLOCKSIZE);
				/* 递归调用自身，走下方 DMA 分支 */
				res = disk_write(SD_CARD, (const BYTE *)scratch, sector++, 1);
				if (res != RES_OK) {
					break;
				}
				buff += SD_BLOCKSIZE;
			}
			return res;
		}

		/*=============================================================*/
		/* 缓冲区已对齐，使用 SDIO DMA 多块写入                        */
		/*=============================================================*/
		SD_state = SD_WriteMultiBlocks((uint8_t *)buff,
									   (uint64_t)sector * SD_BLOCKSIZE,
									   SD_BLOCKSIZE, count);
		if (SD_state == SD_OK) {
			SD_state = SD_WaitWriteOperation();
			while (SD_GetStatus() != SD_TRANSFER_OK);
		}
		if (SD_state != SD_OK)
			stat = RES_PARERR;
		else
			stat = RES_OK;
		break;

	default:
		stat = RES_PARERR;
		break;
	}
	return stat;
}

#endif	/* FF_FS_READONLY == 0 */


/*=========================================================================*/
/*  杂项控制函数                                                           */
/*  功能：处理 FatFs 发出的设备控制命令                                     */
/*  参数：                                                               */
/*    pdrv - 物理驱动器编号                                              */
/*    cmd  - 控制命令（见 diskio.h 中的定义）                             */
/*    buff - 命令参数缓冲区（输入或输出）                                 */
/*  返回：DRESULT                                                       */
/*=========================================================================*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive number */
	BYTE cmd,		/* Control command code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT stat = RES_PARERR;

	switch (pdrv) {
	case SD_CARD:
		switch (cmd) {
		/*=============================================================*/
		/* GET_SECTOR_SIZE：获取扇区大小                                */
		/* 当 FF_MAX_SS > FF_MIN_SS 时，FatFs 通过此命令确认实际扇区大小 */
		/*=============================================================*/
		case GET_SECTOR_SIZE:
			*(WORD *)buff = SD_BLOCKSIZE;
			stat = RES_OK;
			break;

		/*=============================================================*/
		/* GET_BLOCK_SIZE：获取擦除块大小（单位：扇区数）               */
		/* 用于 f_mkfs() 的簇对齐优化，SD 卡通常返回 1                  */
		/*=============================================================*/
		case GET_BLOCK_SIZE:
			*(DWORD *)buff = 1;
			stat = RES_OK;
			break;

		/*=============================================================*/
		/* GET_SECTOR_COUNT：获取总扇区数                               */
		/* 重要！f_mount() 挂载时必须调用此命令获取介质容量              */
		/* SDCardInfo 由 SD_Init() → SD_GetCardInfo() 填充             */
		/*=============================================================*/
		case GET_SECTOR_COUNT:
			*(DWORD *)buff = SDCardInfo.CardCapacity / SDCardInfo.CardBlockSize;
			stat = RES_OK;
			break;

		/*=============================================================*/
		/* CTRL_SYNC：同步/刷缓存                                      */
		/* SD 卡写操作为同步方式，不需要额外刷缓存，直接返回 OK          */
		/*=============================================================*/
		case CTRL_SYNC:
			stat = RES_OK;
			break;

		default:
			stat = RES_PARERR;	/* 不支持的命令 */
			break;
		}
		break;

	default:
		stat = RES_PARERR;
		break;
	}
	return stat;
}


/*=========================================================================*/
/*  获取当前时间戳                                                         */
/*  用途：FatFs 在创建/修改文件时记录时间                                   */
/*  __weak 属性：用户可在其他文件中定义同名的非 weak 函数覆盖此实现          */
/*  如果工程中有 RTC 驱动，实现一个真实的 get_fattime() 即可自动替换        */
/*=========================================================================*/
__weak DWORD get_fattime(void) {
	/* 时间编码格式 (bit)：                                                */
	/*   [31:25] 年 - 1980 = 0~127 => 1980~2107                           */
	/*   [24:21] 月 = 1~12                                                */
	/*   [20:16] 日 = 1~31                                                */
	/*   [15:11] 时 = 0~23                                                */
	/*   [10:5]  分 = 0~59                                                */
	/*   [4:0]   秒 / 2 = 0~29（分辨率 2 秒）                              */
	return	  ((DWORD)(2015 - 1980) << 25)	/* Year 2015 */
			| ((DWORD)1 << 21)				/* Month 1 */
			| ((DWORD)1 << 16)				/* Mday 1 */
			| ((DWORD)0 << 11)				/* Hour 0 */
			| ((DWORD)0 << 5)				/* Min 0 */
			| ((DWORD)0 >> 1);				/* Sec 0 */
}
