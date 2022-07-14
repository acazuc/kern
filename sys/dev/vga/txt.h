#ifndef DEV_VGA_TXT_H
#define DEV_VGA_TXT_H

#include <stdint.h>

struct tty;

void vga_txt_init(uint32_t paddr, uint32_t width, uint32_t height, uint32_t pitch);
int vga_txt_mktty(const char *name, int id, struct tty **tty);

#endif
