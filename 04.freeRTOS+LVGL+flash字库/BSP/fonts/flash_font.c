#include "flash_font.h"
#include "bsp_spi_flash.h"

/* Shared glyph buffer. Aligned(4) because the XBF code casts the returned
 * pointer to uint32_t* to read the pos table entry. Static in RAM. */
static uint8_t __g_font_buf[FLASH_FONT_BUF_SIZE] __attribute__((aligned(4)));

uint8_t * flash_font_getdata(uint32_t flash_base, int offset, int size)
{
    /* XBF offset is relative to the bin base -> add flash_base for W25Q64 */
    SPI_FLASH_BufferRead(__g_font_buf, flash_base + (uint32_t)offset, (uint16_t)size);
    return __g_font_buf;
}
