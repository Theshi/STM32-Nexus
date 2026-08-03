#ifndef __AUX_DATA_H
#define	__AUX_DATA_H

#include "stm32f10x.h"
#include "ff.h"
#include "string.h"


//SD����flash�ĸ�Ŀ¼
#define SD_ROOT       "0:"
#define FLASH_ROOT    "1:"

typedef enum 
{
    UPDATE,
    DO_NOT_UPDATE
}Burn_Option_Typedef;

/* ======== level-2 index: one entry per FILE ======== */
typedef struct
{
  char     name[16];      /* resource name, e.g. "alarm" */
  uint32_t addr;          /* file address in flash */
  uint32_t size;          /* file size (bytes) */
}Index_Enter;

/* ======== level-1 index: one entry per AREA ======== */
typedef struct
{
  uint32_t magic;         /* magic number */
  uint32_t version;       /* version */
  uint32_t entry_num;     /* number of files in this area */
  uint32_t area_addr;     /* area start address */
  uint32_t index_offset;  /* byte offset of this area's index_enter[] from index_enter[0] */
  uint32_t total_size;    /* area total size */
}Index_Group;

/* ======== file description: one entry per FILE to burn ======== */
typedef struct
{
  const char*               name;        /* resource name, same as index_enter.name */
  const char*               filename;    /* SD card file path */
  const char*               description;
  uint32_t                  start_addr;  /* flash start address */
  uint32_t                  length;      /* length, 4KB aligned */
  Burn_Option_Typedef       burn_option;
}Aux_Data_Typedef;


/*
-----------------flash�洢���Ĺ滮-------------------
|��ʼ��ַ |  ��С |         ����                   |
| -------: | ----: | --------------------           |
| 0x000000 |   4KB | Resource Table����ԴĿ¼��      |
| 0x001000 |   4KB | ϵͳ��Ϣ���汾����ԴCRC�ȣ�      |
| 0x002000 |   2MB | ������                          |
| 0x200000 |   1MB | ͼƬ��                          |
| 0x300000 | 512KB | ͼ����                          |
| 0x380000 |   2MB | Ԥ����Դ��                      |
| 0x580000 | 512KB | �û�����                        |
| 0x600000 |   2MB | �����������־                  |
*/

typedef enum 
{
    AUX_DATA_ERROR = -1,
    AUX_ICON_BIN,//ͼ���ļ� 
    AUX_CH_FONT_BIN,//��������
    AUX_ENG_FONT_BIN,//Ӣ������ 
    // AUX_FILE_SYSTEM,      //FATFS�ļ�ϵͳ����f103����ʱ����Ҫ
  
    AUX_MAX_NUM,
} aux_data_t; 

/* ======== resource counts ======== */
#define AUX_ICON_NUM       20   /* icon files */
#define AUX_CH_FONT_NUM     1   /* chinese font files */
#define AUX_EN_FONT_NUM     4   /* english font files */
#define AUX_TOTAL_FILES     (AUX_ICON_NUM + AUX_CH_FONT_NUM + AUX_EN_FONT_NUM)  /* 25 */

extern  Aux_Data_Typedef  burn_data[AUX_TOTAL_FILES];
extern  Index_Enter       index_enter[AUX_TOTAL_FILES];
extern  Index_Group       index_header[AUX_MAX_NUM];


/*��Ϣ���*/
#define BURN_DEBUG_ON         0
#define BURN_DEBUG_FUNC_ON    0

#define BURN_INFO(fmt,arg...)           printf("<<-BURN-INFO->> "fmt"\n",##arg)
#define BURN_ERROR(fmt,arg...)          printf("<<-BURN-ERROR->> "fmt"\n",##arg)
#define BURN_DEBUG(fmt,arg...)          do{\
                                          if(BURN_DEBUG_ON)\
                                          printf("<<-BURN-DEBUG->> [%d]"fmt"\n",__LINE__, ##arg);\
                                          }while(0)

#define BURN_DEBUG_FUNC()               do{\
                                         if(BURN_DEBUG_FUNC_ON)\
                                         printf("<<-BURN-FUNC->> Func:%s@Line:%d\n",__func__,__LINE__);\
                                       }while(0)


#define debug_print_assert(A,B,C,D,E,F) do {if (BURN_DEBUG_ON)\
                                                     printf("\r\nerror code = %d,[occur:%s:%s:%4d] **ASSERT** %s""\r\n",A, D, F, E, (C!=NULL) ? C : "" );\
                                                     }while(0==1)
   
                                       
                                       
 // ==== LOGGING ====
#define SHORT_FILE strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__
 
// ==== BRANCH PREDICTION & EXPRESSION EVALUATION ====
#if( !defined( unlikely ) )
    //#define unlikely( EXPRESSSION )     __builtin_expect( !!(EXPRESSSION), 0 )
   #define unlikely( EXPRESSSION )     !!(EXPRESSSION)
#endif
  

//���ڼ�������err��0����ת��LABEL��                                                     
#if( !defined( check ) )                                       
#define require_noerr( ERR, LABEL )                                                                     \
    do                                                                                                  \
    {                                                                                                   \
        FRESULT        localErr;                                                                       \
                                                                                                        \
        localErr = (FRESULT)(ERR);                                                                     \
        if( unlikely( localErr != 0 ) )                                                                 \
        {                                                                                               \
            debug_print_assert( localErr, NULL, NULL, SHORT_FILE, __LINE__, __PRETTY_FUNCTION__ );        \
            goto LABEL;                                                                                 \
        }                                                                                               \
                                                                                                        \
    }   while( 1==0 )                                       
#endif                                      
                                       
                                       
                                       

FRESULT burn_file_sd2flash(Aux_Data_Typedef *dat,uint8_t file_num);
FRESULT burn_index_table(void);
FRESULT copy_file_sd2flash(char *src_path,char *dst_path);

#endif /* __BURN_DATA_H */
