# FatFS 文件系统移植总结（STM32 + SDIO）

## 一、移植流程

### 1. 获取 FatFS 源码
- 从 FatFS 官网下载 R0.15 源码包，复制以下文件到项目 `FatFS/` 目录：
  - `ff.h` / `ff.c` — FatFS 核心 API
  - `ffconf.h` — 配置文件
  - `diskio.h` / `diskio.c` — 底层磁盘 I/O 接口（需要自己实现）
  - `ffsystem.c` — 操作系统相关接口（多线程同步用，可选）

### 2. 配置 ffconf.h
- `FF_VOLUMES`：逻辑驱动器数量，只用一个 SD 卡设为 `1`
- `FF_FS_READONLY`：是否需要写功能，设为 `0` 开启读写
- `FF_USE_MKFS`：是否需要格式化功能，开发阶段建议 `1`
- `FF_USE_LFN`：长文件名支持，设为 `2`（栈上动态分配）
- `FF_LFN_BUF` / `FF_SFN_BUF`：文件名缓冲区大小，`FF_LFN_BUF` 建议 `255`，`FF_SFN_BUF` 为 `12`，切记 `FF_LFN_BUF >= FF_SFN_BUF`
- `FF_MIN_SS` / `FF_MAX_SS`：扇区大小范围，SD 卡填 `512`
- `FF_FS_NORTC`：无 RTC 时设为 `1`，否则需要实现 `get_fattime()`
- 宏 `FFCONF_DEF` 必须和 `ff.h` 顶部的 `FF_DEFINED` 一致，否则编译报错

### 3. 实现 diskio.c（底层 I/O 函数）

六个必须实现的函数：

| 函数 | 作用 | 关键点 |
|------|------|--------|
| `disk_status(pdrv)` | 获取驱动器状态 | `DSTATUS` 变量必须先初始化为 `0` |
| `disk_initialize(pdrv)` | 初始化驱动器 | 调用 `SD_Init()`，失败返回 `STA_NOINIT` |
| `disk_read(pdrv, buff, sector, count)` | 读扇区 | 注意缓冲区 4 字节对齐，不对齐时用临时缓冲中转 |
| `disk_write(pdrv, buff, sector, count)` | 写扇区 | 受 `FF_FS_READONLY` 条件编译保护 |
| `disk_ioctl(pdrv, cmd, buff)` | 控制命令 | 务必实现 `GET_SECTOR_COUNT`（挂载时必须） |
| `get_fattime()` | 获取当前时间戳 | `__weak` 函数，无 RTC 时返回固定值 |

### 4. 实现 SD 卡驱动（SDIO 接口）
- `SD_Init()` — 初始化 SDIO 外设、GPIO、DMA，识别卡类型（SDSC/SDHC）
- `SD_ReadMultiBlocks()` — DMA 方式多块读取
- `SD_WriteMultiBlocks()` — DMA 方式多块写入
- `SD_WaitReadOperation()` / `SD_WaitWriteOperation()` — 等待 DMA 传输完成
- `SD_GetStatus()` — 查询卡状态（传输完成/忙/错误）

### 5. 将文件加入工程
- 确认以下文件已添加到 IDE 工程中编译：
  - `ff.c`、`ffsystem.c`、`diskio.c`
  - `bsp_sdio_sdcard.c` 以及相关的标准外设库文件
  - 如果需要长文件名支持，还需要 `ffunicode.c`

### 6. 调用 FatFS API 测试
- 先 `f_mount()` 挂载 → `f_open()` 打开文件 → `f_read()`/`f_write()` 读写 → `f_close()` 关闭

---

## 二、本工程中遇到的重点问题

### 1. diskio.c 的花括号嵌套错误（致命）
switch 的闭合花括号被注释掉了，导致 `return stat;` 在 switch 内部，函数体无法正确闭合，编译器报语法错误。
```c
// 错误写法（注释掉了 switch 的闭合花括号）
switch (pdrv) {
case ATA:
    // ...
    break;
//  case SPI_FLASH:
//  break;
//}   ← 这一行被注释掉了，switch 无法闭合
    return stat;
}   ← 这一行实际闭合了 switch，函数体未闭合
```
**解决**：每个 switch 都完整写 `{ case: ... break; default: ... break; }`，不要注释掉结构括号。

### 2. 未初始化的局部变量
```c
DSTATUS stat;        // 错误！stat 未初始化
stat &= ~STA_NODISK; // 对随机值做位运算，结果不可预测
```
**解决**：所有变量声明时立即初始化：`DSTATUS stat = 0;`

### 3. 变量名混用 stat / status
`disk_write()` 和 `disk_ioctl()` 中混用了 `stat` 和 `status` 两个变量名，实际是不同变量但被当作同一变量使用，编译报错。
**解决**：统一使用 `stat`保持命名一致。

### 4. disk_ioctl() 被错误的条件编译屏蔽
`#if _USE_IOCTL` 这个宏从未定义过，导致 `disk_ioctl()` 没有被编译。
但 FatFs R0.15 的 `f_mount()` 在挂载时会调用 `disk_ioctl(GET_SECTOR_COUNT)` 获取总扇区数。
如果该函数不存在，`f_mount()` 在运行时返回 `FR_DISK_ERR`。
**解决**：移除条件编译，`disk_ioctl()` 始终编译。

### 5. FF_LFN_BUF 违反约束
`FF_LFN_BUF` 设为 `5`，`FF_SFN_BUF` 设为 `12`，而 ff.c 要求 `FF_LFN_BUF >= FF_SFN_BUF`。
长文件名缓冲区不可能比短文件名缓冲区小，编译时报 `#error`。
**解决**：`FF_LFN_BUF` 改为 `255`（与 `FF_MAX_LFN` 一致）。

### 6. SPI_FLASH 残留代码
`diskio.c` 中各处残留了 `SPI_FLASH` 的 case 分支和注释掉的代码。
暂只支持 SD 卡时，直接删掉无关分支，保持代码干净。

### 7. SD 卡缓冲区对齐要求
SDIO 的 DMA 要求缓冲区 4 字节对齐（`uint32_t *`），如果传入的 `buff` 不是 4 字节对齐的，直接传给 `SD_ReadMultiBlocks()` 会导致 DMA 传输错误。
**解决**：在 `disk_read()` 和 `disk_write()` 中先检查 `(DWORD)buff & 3`，未对齐时通过栈上的 `DWORD scratch[]` 临时缓冲区中转。

---

## 三、FatFS 使用教程

### 基础步骤

```c
#include "ff.h"

FATFS fs;           // 文件系统对象
FIL file;           // 文件对象
FRESULT res;        // FatFS API 返回值
```

### 1. 挂载文件系统

```c
res = f_mount(&fs, "0:", 1);
if (res == FR_OK) {
    // 挂载成功
} else {
    // 挂载失败，可能是未格式化，需要先格式化
}
```
- 第一个参数：`FATFS` 结构体指针
- 第二个参数：逻辑驱动器号路径，`"0:"` 或 `""` 都代表 0 号驱动器
- 第三个参数：`1` = 立即挂载，`0` = 仅注册，延迟到首次访问时挂载

### 2. 格式化 SD 卡（首次使用或卡未格式化时）

```c
res = f_mkfs("0:", NULL, 0, work_buffer, sizeof(work_buffer));
// work_buffer 需要至少 FF_MAX_SS 大小的缓冲区
```
格式化后需要重新挂载才能使用。

### 3. 打开文件

```c
// 打开已存在的文件用于读取
res = f_open(&file, "0:test.txt", FA_READ);

// 创建新文件或覆盖写入
res = f_open(&file, "0:test.txt", FA_CREATE_ALWAYS | FA_WRITE);

// 打开文件追加写入
res = f_open(&file, "0:test.txt", FA_OPEN_APPEND | FA_WRITE);
```
常用打开模式：
- `FA_READ` — 只读
- `FA_WRITE` — 可写
- `FA_CREATE_NEW` — 创建新文件，已存在则失败
- `FA_CREATE_ALWAYS` — 创建新文件，已存在则覆盖
- `FA_OPEN_ALWAYS` — 打开文件，不存在则创建
- `FA_OPEN_APPEND` — 打开文件，写指针移到末尾

### 4. 读取文件

```c
char buffer[128];
UINT bytes_read;

res = f_open(&file, "0:test.txt", FA_READ);
if (res == FR_OK) {
    res = f_read(&file, buffer, sizeof(buffer) - 1, &bytes_read);
    if (res == FR_OK) {
        buffer[bytes_read] = '\0';  // 字符串结尾
        printf("读取到 %d 字节: %s\n", bytes_read, buffer);
    }
    f_close(&file);
}
```

### 5. 写入文件

```c
const char *data = "Hello FatFS!";
UINT bytes_written;

res = f_open(&file, "0:test.txt", FA_CREATE_ALWAYS | FA_WRITE);
if (res == FR_OK) {
    res = f_write(&file, data, strlen(data), &bytes_written);
    if (res == FR_OK) {
        printf("写入 %d 字节\n", bytes_written);
    }
    f_close(&file);  // 写完后必须关闭（内部会刷新缓存）
}
```

### 6. 目录操作

```c
DIR dir;
FILINFO fno;

res = f_opendir(&dir, "0:/");
if (res == FR_OK) {
    while (1) {
        res = f_readdir(&dir, &fno);
        if (res != FR_OK || fno.fname[0] == 0) break;  // 读取完毕
        if (fno.fattrib & AM_DIR) {
            printf("[DIR]  %s\n", fno.fname);
        } else {
            printf("[FILE] %s  (%lu 字节)\n", fno.fname, fno.fsize);
        }
    }
    f_closedir(&dir);
}
```

### 7. 其他常用操作

```c
// 删除文件
res = f_unlink("0:test.txt");

// 重命名/移动文件
res = f_rename("0:old.txt", "0:new.txt");

// 创建目录
res = f_mkdir("0:data");

// 获取文件信息
res = f_stat("0:test.txt", &fno);

// 获取剩余空间
FATFS *fs;
DWORD free_clusters;
res = f_getfree("0:", &free_clusters, &fs);
DWORD free_bytes = free_clusters * fs->csize * 512;
```

### 8. 完整示例（挂载 → 写入 → 读取）

```c
void FatFs_Test(void) {
    FATFS fs;
    FIL file;
    FRESULT res;
    UINT bw, br;
    char buf[64];

    // 挂载
    res = f_mount(&fs, "0:", 1);
    if (res != FR_OK) {
        printf("挂载失败: %d\n", res);
        return;
    }

    // 写入
    res = f_open(&file, "0:hello.txt", FA_CREATE_ALWAYS | FA_WRITE);
    if (res == FR_OK) {
        f_write(&file, "STM32 FatFS OK!\r\n", 18, &bw);
        f_close(&file);
        printf("写入成功\n");
    }

    // 读取
    res = f_open(&file, "0:hello.txt", FA_READ);
    if (res == FR_OK) {
        f_read(&file, buf, sizeof(buf) - 1, &br);
        buf[br] = '\0';
        printf("读取: %s", buf);
        f_close(&file);
    }

    // 卸载（可选，系统停止前调用）
    f_mount(NULL, "0:", 0);
}
```

### FRESULT 错误码速查

| 返回值 | 含义 | 常见原因 |
|--------|------|----------|
| `FR_OK` | 成功 | — |
| `FR_DISK_ERR` | 底层硬件错误 | `disk_ioctl` 未实现或 SD 卡通信失败 |
| `FR_NOT_READY` | 驱动器未就绪 | `disk_initialize` 返回 `STA_NOINIT` |
| `FR_NO_FILESYSTEM` | 无有效文件系统 | SD 卡未格式化，需 `f_mkfs()` |
| `FR_EXIST` | 文件已存在 | `FA_CREATE_NEW` 但文件已存在 |
| `FR_NOT_ENOUGH_CORE` | 内存不足 | `FF_USE_LFN == 3` 时堆分配失败 |
