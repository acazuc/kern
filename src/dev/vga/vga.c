#include "vga.h"

#include "arch/x86/io.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/std.h>

#define VGA_PTR ((uint16_t*)0xB8000)

#define CRTC_ADDR 0x3D4
#define CRTC_DATA 0x3D5

#define CRTC_ADDR_HTOTAL     0x0
#define CRTC_ADDR_HDSPLY_END 0x1
#define CRTC_ADDR_HBLANK_BEG 0x2
#define CRTC_ADDR_HBLANK_END 0x3
#define CRTC_ADDR_HRTRC_BEG  0x4
#define CRTC_ADDR_HRTRC_END  0x5
#define CRTC_ADDR_VTOTAL     0x6
#define CRTC_ADDR_OVERFLOW   0x7
#define CRTC_ADDR_PRST_RS    0x8
#define CRTC_ADDR_MAX_SL     0x9
#define CRTC_ADDR_CURSOR_BEG 0xA
#define CRTC_ADDR_CURSOR_END 0xB
#define CRTC_ADDR_START_HGH  0xC
#define CRTC_ADDR_START_LOW  0xD
#define CRTC_ADDR_CURSOR_HGH 0xE
#define CRTC_ADDR_CURSOR_LOW 0xF
#define CRTC_ADDR_VRTRC_BEG  0x10
#define CRTC_ADDR_VRTRC_END  0x11
#define CRTC_ADDR_VDSPLY_END 0x12
#define CRTC_ADDR_OFFSET     0x13
#define CRTC_ADDR_UNDERLINE  0x14
#define CRTC_ADDR_VBLANK_BEG 0x15
#define CRTC_ADDR_VBLANK_END 0x16
#define CRTC_ADDR_CRTC_MODE  0x17
#define CRTC_ADDR_LINE_CMP   0x18

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) 
{
	return (uint16_t)uc | (uint16_t)color << 8;
}

void vga_init()
{
	uint16_t clear_entry = vga_entry(' ', vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_BLACK));
	for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; ++i)
		VGA_PTR[i] = clear_entry;
}

void vga_set_char(uint8_t x, uint8_t y, char c, uint8_t color)
{
	vga_set_val(x, y, vga_entry(c, color));
}

uint16_t vga_get_val(uint8_t x, uint8_t y)
{
	return VGA_PTR[y * VGA_WIDTH + x];
}

void vga_set_val(uint8_t x, uint8_t y, uint16_t val)
{
	VGA_PTR[y * VGA_WIDTH + x] = val;
}

void vga_set_cursor(uint8_t x, uint8_t y)
{
	uint16_t p = y * (uint16_t)VGA_WIDTH + x;
	outb(CRTC_ADDR, CRTC_ADDR_CURSOR_HGH);
	outb(CRTC_DATA, (p >> 8) & 0xFF);
	outb(CRTC_ADDR, CRTC_ADDR_CURSOR_LOW);
	outb(CRTC_DATA, p & 0xFF);
}

void vga_enable_cursor()
{
	outb(CRTC_ADDR, CRTC_ADDR_CURSOR_BEG);
	outb(CRTC_DATA, (inb(CRTC_DATA) & 0xC0) | 0x20 | 10);
	outb(CRTC_ADDR, CRTC_ADDR_CURSOR_END);
	outb(CRTC_DATA, (inb(CRTC_DATA) & 0xE0) | 15);
}

void vga_disable_cursor()
{
}
