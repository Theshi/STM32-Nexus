/**
  ******************************************************************************
  * @file    stm32_eval_sdio_sd.h
  * @author  MCD Application Team
  * @version V4.5.0
  * @date    07-March-2011
  * @brief   This file contains all the functions prototypes for the SD Card 
  *          stm32_eval_sdio_sd driver firmware library.
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************  
  */ 
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SDIO_SDCARD_H
#define __SDIO_SDCARD_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"

/* Exported types ------------------------------------------------------------*/
typedef enum
{
/** 
  * @brief  SDIO specific error defines  
  */   
  SD_CMD_CRC_FAIL                    = (1), /*!< Command response received (but CRC check failed) */
  SD_DATA_CRC_FAIL                   = (2), /*!< Data bock sent/received (CRC check Failed) */
  SD_CMD_RSP_TIMEOUT                 = (3), /*!< Command response timeout */
  SD_DATA_TIMEOUT                    = (4), /*!< Data time out */
  SD_TX_UNDERRUN                     = (5), /*!< Transmit FIFO under-run */
  SD_RX_OVERRUN                      = (6), /*!< Receive FIFO over-run */
  SD_START_BIT_ERR                   = (7), /*!< Start bit not detected on all data signals in widE bus mode */
  SD_CMD_OUT_OF_RANGE                = (8), /*!< CMD's argument was out of range.*/
  SD_ADDR_MISALIGNED                 = (9), /*!< Misaligned address */
  SD_BLOCK_LEN_ERR                   = (10), /*!< Transferred block length is not allowed for the card or the number of transferred bytes does not match the block length */
  SD_ERASE_SEQ_ERR                   = (11), /*!< An error in the sequence of erase command occurs.*/
  SD_BAD_ERASE_PARAM                 = (12), /*!< An Invalid selection for erase groups */
  SD_WRITE_PROT_VIOLATION            = (13), /*!< Attempt to program a write protect block */
  SD_LOCK_UNLOCK_FAILED              = (14), /*!< Sequence or password error has been detected in unlock command or if there was an attempt to access a locked card */
  SD_COM_CRC_FAILED                  = (15), /*!< CRC check of the previous command failed */
  SD_ILLEGAL_CMD                     = (16), /*!< Command is not legal for the card state */
  SD_CARD_ECC_FAILED                 = (17), /*!< Card internal ECC was applied but failed to correct the data */
  SD_CC_ERROR                        = (18), /*!< Internal card controller error */
  SD_GENERAL_UNKNOWN_ERROR           = (19), /*!< General or Unknown error */
  SD_STREAM_READ_UNDERRUN            = (20), /*!< The card could not sustain data transfer in stream read operation. */
  SD_STREAM_WRITE_OVERRUN            = (21), /*!< The card could not sustain data programming in stream mode */
  SD_CID_CSD_OVERWRITE               = (22), /*!< CID/CSD overwrite error */
  SD_WP_ERASE_SKIP                   = (23), /*!< only partial address space was erased */
  SD_CARD_ECC_DISABLED               = (24), /*!< Command has been executed without using internal ECC */
  SD_ERASE_RESET                     = (25), /*!< Erase sequence was cleared before executing because an out of erase sequence command was received */
  SD_AKE_SEQ_ERROR                   = (26), /*!< Error in sequence of authentication. */
  SD_INVALID_VOLTRANGE               = (27),
  SD_ADDR_OUT_OF_RANGE               = (28),
  SD_SWITCH_ERROR                    = (29),
  SD_SDIO_DISABLED                   = (30),
  SD_SDIO_FUNCTION_BUSY              = (31),
  SD_SDIO_FUNCTION_FAILED            = (32),
  SD_SDIO_UNKNOWN_FUNCTION           = (33),

/** 
  * @brief  Standard error defines   
  */ 
  SD_INTERNAL_ERROR, 
  SD_NOT_CONFIGURED,
  SD_REQUEST_PENDING, 
  SD_REQUEST_NOT_APPLICABLE, 
  SD_INVALID_PARAMETER,  
  SD_UNSUPPORTED_FEATURE,  
  SD_UNSUPPORTED_HW,  
  SD_ERROR,  
  SD_OK = 0 
} SD_Error;

/** 
  * @brief  SDIO Transfer state  
  */   
typedef enum
{
  SD_TRANSFER_OK  = 0,
  SD_TRANSFER_BUSY = 1,
  SD_TRANSFER_ERROR
} SDTransferState;

/** 
  * @brief  SD Card States 
  */   
typedef enum
{
  SD_CARD_READY                  = ((uint32_t)0x00000001),
  SD_CARD_IDENTIFICATION         = ((uint32_t)0x00000002),
  SD_CARD_STANDBY                = ((uint32_t)0x00000003),
  SD_CARD_TRANSFER               = ((uint32_t)0x00000004),
  SD_CARD_SENDING                = ((uint32_t)0x00000005),
  SD_CARD_RECEIVING              = ((uint32_t)0x00000006),
  SD_CARD_PROGRAMMING            = ((uint32_t)0x00000007),
  SD_CARD_DISCONNECTED           = ((uint32_t)0x00000008),
  SD_CARD_ERROR                  = ((uint32_t)0x000000FF)
}SDCardState;


/** 
  * @brief  Card Specific Data: CSD Register   
  */ 
typedef struct
{
  __IO uint8_t  CSDStruct;            /*!< CSD structure */
  __IO uint8_t  SysSpecVersion;       /*!< System specification version */
  __IO uint8_t  Reserved1;            /*!< Reserved */
  __IO uint8_t  TAAC;                 /*!< Data read access-time 1 */
  __IO uint8_t  NSAC;                 /*!< Data read access-time 2 in CLK cycles */
  __IO uint8_t  MaxBusClkFrec;        /*!< Max. bus clock frequency */
  __IO uint16_t CardComdClasses;      /*!< Card command classes */
  __IO uint8_t  RdBlockLen;           /*!< Max. read data block length */
  __IO uint8_t  PartBlockRead;        /*!< Partial blocks for read allowed */
  __IO uint8_t  WrBlockMisalign;      /*!< Write block misalignment */
  __IO uint8_t  RdBlockMisalign;      /*!< Read block misalignment */
  __IO uint8_t  DSRImpl;              /*!< DSR implemented */
  __IO uint8_t  Reserved2;            /*!< Reserved */
  __IO uint32_t DeviceSize;           /*!< Device Size */
  __IO uint8_t  MaxRdCurrentVDDMin;   /*!< Max. read current @ VDD min */
  __IO uint8_t  MaxRdCurrentVDDMax;   /*!< Max. read current @ VDD max */
  __IO uint8_t  MaxWrCurrentVDDMin;   /*!< Max. write current @ VDD min */
  __IO uint8_t  MaxWrCurrentVDDMax;   /*!< Max. write current @ VDD max */
  __IO uint8_t  DeviceSizeMul;        /*!< Device size multiplier */
  __IO uint8_t  EraseGrSize;          /*!< Erase group size */
  __IO uint8_t  EraseGrMul;           /*!< Erase group size multiplier */
  __IO uint8_t  WrProtectGrSize;      /*!< Write protect group size */
  __IO uint8_t  WrProtectGrEnable;    /*!< Write protect group enable */
  __IO uint8_t  ManDeflECC;           /*!< Manufacturer default ECC */
  __IO uint8_t  WrSpeedFact;          /*!< Write speed factor */
  __IO uint8_t  MaxWrBlockLen;        /*!< Max. write data block length */
  __IO uint8_t  WriteBlockPaPartial;  /*!< Partial blocks for write allowed */
  __IO uint8_t  Reserved3;            /*!< Reserded */
  __IO uint8_t  ContentProtectAppli;  /*!< Content protection application */
  __IO uint8_t  FileFormatGrouop;     /*!< File format group */
  __IO uint8_t  CopyFlag;             /*!< Copy flag (OTP) */
  __IO uint8_t  PermWrProtect;        /*!< Permanent write protection */
  __IO uint8_t  TempWrProtect;        /*!< Temporary write protection */
  __IO uint8_t  FileFormat;           /*!< File Format */
  __IO uint8_t  ECC;                  /*!< ECC code */
  __IO uint8_t  CSD_CRC;              /*!< CSD CRC */
  __IO uint8_t  Reserved4;            /*!< always 1*/
} SD_CSD;

/** 
  * @brief  Card Identification Data: CID Register   
  */
typedef struct
{
  __IO uint8_t  ManufacturerID;       /*!< ManufacturerID */
  __IO uint16_t OEM_AppliID;          /*!< OEM/Application ID */
  __IO uint32_t ProdName1;            /*!< Product Name part1 */
  __IO uint8_t  ProdName2;            /*!< Product Name part2*/
  __IO uint8_t  ProdRev;              /*!< Product Revision */
  __IO uint32_t ProdSN;               /*!< Product Serial Number */
  __IO uint8_t  Reserved1;            /*!< Reserved1 */
  __IO uint16_t ManufactDate;         /*!< Manufacturing Date */
  __IO uint8_t  CID_CRC;              /*!< CID CRC */
  __IO uint8_t  Reserved2;            /*!< always 1 */
} SD_CID;

/** 
  * @brief SD Card Status 
  */
typedef struct
{
  __IO uint8_t DAT_BUS_WIDTH;
  __IO uint8_t SECURED_MODE;
  __IO uint16_t SD_CARD_TYPE;
  __IO uint32_t SIZE_OF_PROTECTED_AREA;
  __IO uint8_t SPEED_CLASS;
  __IO uint8_t PERFORMANCE_MOVE;
  __IO uint8_t AU_SIZE;
  __IO uint16_t ERASE_SIZE;
  __IO uint8_t ERASE_TIMEOUT;
  __IO uint8_t ERASE_OFFSET;
} SD_CardStatus;


/** 
  * @brief SD Card information 
  */
typedef struct
{
  SD_CSD SD_csd;
  SD_CID SD_cid;
  uint64_t CardCapacity;  /*!< Card Capacity */
  uint32_t CardBlockSize; /*!< Card Block Size */
  uint16_t RCA;
  uint8_t CardType;
} SD_CardInfo;

/*�궨��*/
#define SDIO_FIFO_ADDRESS                ((uint32_t)0x40018080)	 //SDIO_FIOF��ַ=SDIO��ַ+0x80�� sdio��ַ+0xfc
/** 
  * @brief  SDIO Intialization Frequency (400KHz max)
  */
#define SDIO_INIT_CLK_DIV                ((uint8_t)0xB2)
/** 
  * @brief  SDIO Data Transfer Frequency (25MHz max) 
  */
/*!< SDIOCLK = HCLK, SDIO_CK = HCLK/(2 + SDIO_TRANSFER_CLK_DIV) */
#define SDIO_TRANSFER_CLK_DIV            ((uint8_t)0x01) 

	  
/**
  * @brief SDIO Commands Index
  *         这些是 SD 物理层协议定义的命令索引号。
  *         每个命令由主机通过 CMD 线发送，卡在响应线返回应答。
  *
  *         SD 命令格式: [起始位(0)] [传输位(1)] [命令索引(6bit)] [参数(32bit)] [CRC7(7bit)] [结束位(1)]
  *
  *         命令分类:
  *         - 基本命令 (CMD0~CMD7):  卡识别和选择
  *         - 读传输命令 (CMD8~CMD18):   读取数据
  *         - 写传输命令 (CMD20~CMD27):  写入数据
  *         - 擦除命令 (CMD32~CMD38): 擦除操作
  *         - 应用相关命令 (CMD55~CMD56): 需配合 ACMD 使用
  *
  *         注意: ACMD 是"应用专用命令"，发送 ACMD 前必须先发 CMD55。
  */
#define SD_CMD_GO_IDLE_STATE                       ((uint8_t)0)   /* CMD0:  复位卡到空闲状态。所有卡初始化从这里开始，无响应 */
#define SD_CMD_SEND_OP_COND                        ((uint8_t)1)  /* CMD1:  发送操作条件(主要用于MMC)，R3响应返回OCR寄存器 */
#define SD_CMD_ALL_SEND_CID                        ((uint8_t)2)  /* CMD2:  请求所有卡返回CID(卡标识符)，R2长响应 */
#define SD_CMD_SET_REL_ADDR                        ((uint8_t)3)  /* CMD3:  设置/获取卡相对地址(RCA)，R6响应返回RCA，SD卡特有 */
#define SD_CMD_SET_DSR                             ((uint8_t)4)  /* CMD4:  设置DSR(驱动级寄存器)，一般不使用 */
#define SD_CMD_SDIO_SEN_OP_COND                    ((uint8_t)5)  /* CMD5:  SDIO卡的操作条件查询，仅用于SDIO卡 */
#define SD_CMD_HS_SWITCH                           ((uint8_t)6)  /* CMD6:  切换卡的功能模式(如高速度、总线宽度) */
#define SD_CMD_SEL_DESEL_CARD                      ((uint8_t)7)  /* CMD7:  选中/取消选中卡(通过RCA)，选中后进入传输状态 */
#define SD_CMD_HS_SEND_EXT_CSD                     ((uint8_t)8)  /* CMD8:  SEND_IF_COND，发送接口条件，检测SD 2.0卡 */
#define SD_CMD_SEND_CSD                            ((uint8_t)9)  /* CMD9:  读CSD(卡特定数据)寄存器，R2长响应，含容量信息 */
#define SD_CMD_SEND_CID                            ((uint8_t)10) /* CMD10: 读CID(卡标识符)寄存器，R2长响应 */
#define SD_CMD_READ_DAT_UNTIL_STOP                 ((uint8_t)11) /* CMD11: 连续读直到停止(SD卡不支持) */
#define SD_CMD_STOP_TRANSMISSION                   ((uint8_t)12) /* CMD12: 停止正在进行的多块读写传输 */
#define SD_CMD_SEND_STATUS                         ((uint8_t)13) /* CMD13: 查询卡状态寄存器(R1)，获取当前卡状态 */
#define SD_CMD_HS_BUSTEST_READ                     ((uint8_t)14) /* CMD14: 高速总线测试读 */
#define SD_CMD_GO_INACTIVE_STATE                   ((uint8_t)15) /* CMD15: 使卡进入非活跃状态 */
#define SD_CMD_SET_BLOCKLEN                        ((uint8_t)16) /* CMD16: 设置块长度(对SDSC有效，SDHC/SDXC固定512字节) */
#define SD_CMD_READ_SINGLE_BLOCK                   ((uint8_t)17) /* CMD17: 读单块——从指定地址读一个512字节块 */
#define SD_CMD_READ_MULT_BLOCK                     ((uint8_t)18) /* CMD18: 读多块——连续读取直到收到CMD12停止 */
#define SD_CMD_HS_BUSTEST_WRITE                    ((uint8_t)19) /* CMD19: 高速总线测试写 */
#define SD_CMD_WRITE_DAT_UNTIL_STOP                ((uint8_t)20) /* CMD20: 连续写直到停止(SD卡不支持) */
#define SD_CMD_SET_BLOCK_COUNT                     ((uint8_t)23) /* CMD23: 设置多块传输的块数(配合CMD18/25使用) */
#define SD_CMD_WRITE_SINGLE_BLOCK                  ((uint8_t)24) /* CMD24: 写单块——向指定地址写一个512字节块 */
#define SD_CMD_WRITE_MULT_BLOCK                    ((uint8_t)25) /* CMD25: 写多块——连续写入多块数据 */
#define SD_CMD_PROG_CID                            ((uint8_t)26) /* CMD26: 编程CID(厂家使用) */
#define SD_CMD_PROG_CSD                            ((uint8_t)27) /* CMD27: 编程CSD(厂家使用) */
#define SD_CMD_SET_WRITE_PROT                      ((uint8_t)28) /* CMD28: 设置写保护 */
#define SD_CMD_CLR_WRITE_PROT                      ((uint8_t)29) /* CMD29: 清除写保护 */
#define SD_CMD_SEND_WRITE_PROT                     ((uint8_t)30) /* CMD30: 查询写保护状态 */
#define SD_CMD_SD_ERASE_GRP_START                  ((uint8_t)32) /* CMD32: 设置擦除起始地址(SD卡专用) */
#define SD_CMD_SD_ERASE_GRP_END                    ((uint8_t)33) /* CMD33: 设置擦除结束地址(SD卡专用) */
#define SD_CMD_ERASE_GRP_START                     ((uint8_t)35) /* CMD35: 设置擦除起始地址(MMC卡专用) */
#define SD_CMD_ERASE_GRP_END                       ((uint8_t)36) /* CMD36: 设置擦除结束地址(MMC卡专用) */
#define SD_CMD_ERASE                               ((uint8_t)38) /* CMD38: 执行擦除——擦除CMD32~CMD33指定的区域 */

#define SD_CMD_FAST_IO                             ((uint8_t)39) /* CMD39: 快速I/O(SD卡不支持) */
#define SD_CMD_GO_IRQ_STATE                        ((uint8_t)40) /* CMD40: 进入IRQ状态(SD卡不支持) */
#define SD_CMD_LOCK_UNLOCK                         ((uint8_t)42) /* CMD42: 锁定/解锁卡 */
#define SD_CMD_APP_CMD                             ((uint8_t)55) /* CMD55: 告诉卡下一条是应用命令(ACMD) */
#define SD_CMD_GEN_CMD                             ((uint8_t)56) /* CMD56: 通用命令，传输数据块 */
#define SD_CMD_NO_CMD                              ((uint8_t)64) /* 空命令，无操作 */

/**
  * @brief Following commands are SD Card Specific commands.
  *        SDIO_APP_CMD (CMD55) should be sent before sending these commands.
  *        这些是"应用专用命令"(ACMD)，发之前必须先发 CMD55。
  *        卡收到 CMD55 后，把下一条命令解析为 ACMD。
  *        例如: CMD55 + CMD41 = ACMD41。
  */
#define SD_CMD_APP_SD_SET_BUSWIDTH                 ((uint8_t)6)  /* ACMD6:  设置总线宽度(0=1bit, 2=4bit) */
#define SD_CMD_SD_APP_STAUS                        ((uint8_t)13) /* ACMD13: 查询SD卡状态(SD Status) */
#define SD_CMD_SD_APP_SEND_NUM_WRITE_BLOCKS        ((uint8_t)22) /* ACMD22: 查询已写入块数 */
#define SD_CMD_SD_APP_OP_COND                      ((uint8_t)41) /* ACMD41: 发送操作条件，协商电压+检测SDHC */
#define SD_CMD_SD_APP_SET_CLR_CARD_DETECT          ((uint8_t)42) /* ACMD42: 设置/清除卡检测 */
#define SD_CMD_SD_APP_SEND_SCR                     ((uint8_t)51) /* ACMD51: 读取SCR配置寄存器 */
#define SD_CMD_SDIO_RW_DIRECT                      ((uint8_t)52) /* CMD52: SDIO卡直接读写(单字节) */
#define SD_CMD_SDIO_RW_EXTENDED                    ((uint8_t)53) /* CMD53: SDIO卡扩展读写(多字节/FIFO) */

/** 
  * @brief Following commands are SD Card Specific security commands.
  *        SDIO_APP_CMD should be sent before sending these commands. 
  */
#define SD_CMD_SD_APP_GET_MKB                      ((uint8_t)43) /*!< For SD Card only */
#define SD_CMD_SD_APP_GET_MID                      ((uint8_t)44) /*!< For SD Card only */
#define SD_CMD_SD_APP_SET_CER_RN1                  ((uint8_t)45) /*!< For SD Card only */
#define SD_CMD_SD_APP_GET_CER_RN2                  ((uint8_t)46) /*!< For SD Card only */
#define SD_CMD_SD_APP_SET_CER_RES2                 ((uint8_t)47) /*!< For SD Card only */
#define SD_CMD_SD_APP_GET_CER_RES1                 ((uint8_t)48) /*!< For SD Card only */
#define SD_CMD_SD_APP_SECURE_READ_MULTIPLE_BLOCK   ((uint8_t)18) /*!< For SD Card only */
#define SD_CMD_SD_APP_SECURE_WRITE_MULTIPLE_BLOCK  ((uint8_t)25) /*!< For SD Card only */
#define SD_CMD_SD_APP_SECURE_ERASE                 ((uint8_t)38) /*!< For SD Card only */
#define SD_CMD_SD_APP_CHANGE_SECURE_AREA           ((uint8_t)49) /*!< For SD Card only */
#define SD_CMD_SD_APP_SECURE_WRITE_MKB             ((uint8_t)48) /*!< For SD Card only */
  
/* Uncomment the following line to select the SDIO Data transfer mode */  
#define SD_DMA_MODE                                ((uint32_t)0x00000000)
/*#define SD_POLLING_MODE                            ((uint32_t)0x00000002)*/

/**
  * @brief  SD detection on its memory slot
  */
#define SD_PRESENT                                 ((uint8_t)0x01)
#define SD_NOT_PRESENT                             ((uint8_t)0x00)

/** 
  * @brief Supported SD Memory Cards 
  */
#define SDIO_STD_CAPACITY_SD_CARD_V1_1             ((uint32_t)0x00000000)
#define SDIO_STD_CAPACITY_SD_CARD_V2_0             ((uint32_t)0x00000001)
#define SDIO_HIGH_CAPACITY_SD_CARD                 ((uint32_t)0x00000002)
#define SDIO_MULTIMEDIA_CARD                       ((uint32_t)0x00000003)
#define SDIO_SECURE_DIGITAL_IO_CARD                ((uint32_t)0x00000004)
#define SDIO_HIGH_SPEED_MULTIMEDIA_CARD            ((uint32_t)0x00000005)
#define SDIO_SECURE_DIGITAL_IO_COMBO_CARD          ((uint32_t)0x00000006)
#define SDIO_HIGH_CAPACITY_MMC_CARD                ((uint32_t)0x00000007)


/* Exported functions ------------------------------------------------------- */
void SD_DeInit(void);
SD_Error SD_Init(void);
SDTransferState SD_GetStatus(void);
SDCardState SD_GetState(void);
uint8_t SD_Detect(void);
SD_Error SD_PowerON(void);
SD_Error SD_PowerOFF(void);
SD_Error SD_InitializeCards(void);
SD_Error SD_GetCardInfo(SD_CardInfo *cardinfo);
SD_Error SD_GetCardStatus(SD_CardStatus *cardstatus);
SD_Error SD_EnableWideBusOperation(uint32_t WideMode);
SD_Error SD_SelectDeselect(uint32_t addr);
SD_Error SD_ReadBlock(uint8_t *readbuff, uint64_t ReadAddr, uint16_t BlockSize);
SD_Error SD_ReadMultiBlocks(uint8_t *readbuff, uint64_t ReadAddr, uint16_t BlockSize, uint32_t NumberOfBlocks);
SD_Error SD_WriteBlock(uint8_t *writebuff, uint64_t WriteAddr, uint16_t BlockSize);
SD_Error SD_WriteMultiBlocks(uint8_t *writebuff, uint64_t WriteAddr, uint16_t BlockSize, uint32_t NumberOfBlocks);
SDTransferState SD_GetTransferState(void);
SD_Error SD_StopTransfer(void);
SD_Error SD_Erase(uint32_t startaddr, uint32_t endaddr);
SD_Error SD_SendStatus(uint32_t *pcardstatus);
SD_Error SD_SendSDStatus(uint32_t *psdstatus);
SD_Error SD_ProcessIRQSrc(void);
SD_Error SD_WaitReadOperation(void);
SD_Error SD_WaitWriteOperation(void);



#endif /* __SDCARD_H */

