#include "vga.h"

void vga_txt_init(uint32_t paddr, uint32_t width, uint32_t height, uint32_t pitch);
void vga_rgb_init(uint32_t paddr, uint32_t width, uint32_t height, uint32_t pitch, uint32_t bpp);

int vga_txt_mktty(const char *name, int id, struct tty **tty);
int vga_rgb_mktty(const char *name, int id, struct tty **tty);

struct vga *g_vga;

void vga_init(int rgb, uint32_t paddr, uint32_t width, uint32_t height, uint32_t pitch, uint32_t bpp)
{
	if (rgb)
		vga_rgb_init(paddr, width, height, pitch, bpp);
	else
		vga_txt_init(paddr, width, height, pitch);
}

int vga_mktty(const char *name, int id, struct tty **tty)
{
	if (g_vga->rgb)
		return vga_rgb_mktty(name, id, tty);
	return vga_txt_mktty(name, id, tty);
}
