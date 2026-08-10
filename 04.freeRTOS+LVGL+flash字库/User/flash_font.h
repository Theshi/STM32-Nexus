#ifndef __FLASH_FONT_H
#define __FLASH_FONT_H

#include <stdint.h>

/* Shared glyph buffer size: >= largest glyph bitmap in the 5 burned fonts.
 * max = 1710B (my_font_ENG_UI_36's 60x57@4bpp FontAwesome icon 0xF000)   */
#define FLASH_FONT_BUF_SIZE  2048

/* Read `size` bytes at `offset` of a font bin from W25Q64 into a shared
 * static buffer and return the buffer pointer.
 * `offset` is RELATIVE to the font bin base; `flash_base` is that bin's
 * absolute address in W25Q64 (see the resource index table / burn_data[]).
 * LVGL renders one glyph at a time, so all fonts share one buffer to save RAM. */
uint8_t * flash_font_getdata(uint32_t flash_base, int offset, int size);

#endif /* __FLASH_FONT_H */
