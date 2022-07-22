#ifndef DEV_VGA_H
#define DEV_VGA_H

#include <stdint.h>

struct tty;

struct vga_op
{
};

struct vga
{
	const struct vga_op *op;
	int rgb;
	uint32_t paddr;
	uint32_t width;
	uint32_t height;
	uint32_t pitch;
	uint32_t bpp;
};

extern struct vga *g_vga;

void vga_init(int rgb, uint32_t paddr, uint32_t width, uint32_t height, uint32_t pitch, uint32_t bpp);
int vga_mktty(const char *name, int id, struct tty **tty);

#endif
