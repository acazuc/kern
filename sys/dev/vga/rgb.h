#ifndef DEV_VGA_RGB_H
#define DEV_VGA_RGB_H

#include <stdint.h>

void vga_rgb_init(uint32_t paddr, uint32_t width, uint32_t height, uint32_t pitch, uint32_t bpp);

#endif
