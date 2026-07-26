#include "stm32f10x.h"                  // Device header
#include "W25Q64.h"
#include "lfs.h"

/* ---------------------包装函数（在 cfg 前面，否则 ARMCC 会报警告）-----------------*/
static int lfs_read(const struct lfs_config *c,
                    lfs_block_t block, lfs_off_t off,
                    void *buffer, lfs_size_t size)
{
    uint32_t addr = block * c->block_size + off;
    W25Q64_Read((uint32_t)addr, (uint8_t *)buffer, size);
    return 0;
}

static int lfs_prog(const struct lfs_config *c,
                    lfs_block_t block, lfs_off_t off,
                    const void *buffer, lfs_size_t size)
{
    uint32_t addr = block * c->block_size + off;
    const uint8_t *buf = (const uint8_t *)buffer;
    uint32_t remain = size;
    while (remain > 0) {
        uint32_t n = (remain > 256) ? 256 : remain;
        W25Q64_Write(addr, (uint8_t *)buf, (uint16_t)n);
        addr += n;
        buf += n;
        remain -= n;
    }
    return 0;
}

static int lfs_erase(const struct lfs_config *c, lfs_block_t block)
{
    uint32_t addr = block * c->block_size;
    W25Q64_SectorErase(addr);
    return 0;
}

static int lfs_sync(const struct lfs_config *c)
{
    W25Q64_WaitBusy();
    return 0;
}

/* ---- lfs_config 实例 ---- */
struct lfs_config cfg = {
    .read_size      = 16,
    .prog_size      = 16,
    .block_size     = 4096,
    .block_count    = 2048,
    .cache_size     = 256,
    .lookahead_size = 64,
    .block_cycles   = 500,

    .read  = lfs_read,
    .prog  = lfs_prog,
    .erase = lfs_erase,
    .sync  = lfs_sync,
};
