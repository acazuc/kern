#include "txt.h"
#include "arch/arch.h"
#include "arch/x86/asm.h"
#include "dev/tty/tty.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>

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

struct vga_txt
{
	uint16_t *addr;
	size_t size;
	uint32_t width;
	uint32_t height;
	uint32_t pitch;
};

static struct vga_txt g_vga_txt;

enum vga_color
{
	VGA_COLOR_BLACK         = 0x0,
	VGA_COLOR_BLUE          = 0x1,
	VGA_COLOR_GREEN         = 0x2,
	VGA_COLOR_CYAN          = 0x3,
	VGA_COLOR_RED           = 0x4,
	VGA_COLOR_MAGENTA       = 0x5,
	VGA_COLOR_BROWN         = 0x6,
	VGA_COLOR_LIGHT_GREY    = 0x7,
	VGA_COLOR_DARK_GREY     = 0x8,
	VGA_COLOR_LIGHT_BLUE    = 0x9,
	VGA_COLOR_LIGHT_GREEN   = 0xA,
	VGA_COLOR_LIGHT_CYAN    = 0xB,
	VGA_COLOR_LIGHT_RED     = 0xC,
	VGA_COLOR_LIGHT_MAGENTA = 0xD,
	VGA_COLOR_LIGHT_BROWN   = 0xE,
	VGA_COLOR_WHITE         = 0xF,
};

static void set_val(uint8_t x, uint8_t y, uint16_t val);

static inline uint8_t entry_color(enum vga_color fg, enum vga_color bg) 
{
	return fg | bg << 4;
}

static inline uint16_t mkentry(unsigned char uc, uint8_t color) 
{
	return (uint16_t)uc | (uint16_t)color << 8;
}

void vga_txt_init(uint32_t paddr, uint32_t width, uint32_t height, uint32_t pitch)
{
	g_vga_txt.size = height * pitch;
	g_vga_txt.size += PAGE_SIZE - 1;
	g_vga_txt.size -= g_vga_txt.size % PAGE_SIZE;
	g_vga_txt.addr = vmap(paddr, g_vga_txt.size);
	g_vga_txt.width = width;
	g_vga_txt.height = height;
	g_vga_txt.pitch = pitch / 2;
	uint16_t clear_entry = mkentry(' ', entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
	for (size_t y = 0; y < height; ++y)
	{
		for (size_t x = 0; x < width; ++x)
			set_val(x, y, clear_entry);
	}
}

static void set_char(uint8_t x, uint8_t y, char c, uint8_t color)
{
	set_val(x, y, mkentry(c, color));
}

static uint16_t get_val(uint8_t x, uint8_t y)
{
	return g_vga_txt.addr[y * g_vga_txt.pitch + x];
}

static void set_val(uint8_t x, uint8_t y, uint16_t val)
{
	g_vga_txt.addr[y * g_vga_txt.pitch + x] = val;
}

static void set_cursor(uint8_t x, uint8_t y)
{
	uint16_t p = y * g_vga_txt.width + x;
	outb(CRTC_ADDR, CRTC_ADDR_CURSOR_HGH);
	outb(CRTC_DATA, (p >> 8) & 0xFF);
	outb(CRTC_ADDR, CRTC_ADDR_CURSOR_LOW);
	outb(CRTC_DATA, p & 0xFF);
}

static void enable_cursor(void)
{
	outb(CRTC_ADDR, CRTC_ADDR_CURSOR_BEG);
	outb(CRTC_DATA, (inb(CRTC_DATA) & 0xC0) | 13);
	outb(CRTC_ADDR, CRTC_ADDR_CURSOR_END);
	outb(CRTC_DATA, (inb(CRTC_DATA) & 0xE0) | 15);
}

static void disable_cursor(void)
{
	outb(CRTC_ADDR, CRTC_ADDR_CURSOR_BEG);
	outb(CRTC_DATA, 0x20);
}

struct vga_txt_tty
{
	uint8_t row;
	uint8_t column;
	uint8_t color;
	uint8_t fg_color;
	uint8_t bg_color;
	uint8_t bold;
};

static const uint8_t g_fg_colors[] =
{
	VGA_COLOR_BLACK,
	VGA_COLOR_RED,
	VGA_COLOR_GREEN,
	VGA_COLOR_BROWN,
	VGA_COLOR_BLUE,
	VGA_COLOR_MAGENTA,
	VGA_COLOR_CYAN,
	VGA_COLOR_LIGHT_GREY,
	VGA_COLOR_LIGHT_GREY,
	VGA_COLOR_LIGHT_GREY,
	VGA_COLOR_DARK_GREY,
	VGA_COLOR_LIGHT_RED,
	VGA_COLOR_LIGHT_GREEN,
	VGA_COLOR_LIGHT_BROWN,
	VGA_COLOR_LIGHT_BLUE,
	VGA_COLOR_LIGHT_MAGENTA,
	VGA_COLOR_LIGHT_CYAN,
	VGA_COLOR_WHITE,
	VGA_COLOR_WHITE,
	VGA_COLOR_WHITE
};

static const uint8_t g_bg_colors[] =
{
	VGA_COLOR_BLACK,
	VGA_COLOR_RED,
	VGA_COLOR_GREEN,
	VGA_COLOR_BROWN,
	VGA_COLOR_BLUE,
	VGA_COLOR_MAGENTA,
	VGA_COLOR_CYAN,
	VGA_COLOR_LIGHT_GREY,
	VGA_COLOR_BLACK,
	VGA_COLOR_BLACK,
	VGA_COLOR_DARK_GREY,
	VGA_COLOR_LIGHT_RED,
	VGA_COLOR_LIGHT_GREEN,
	VGA_COLOR_LIGHT_BROWN,
	VGA_COLOR_LIGHT_BLUE,
	VGA_COLOR_LIGHT_MAGENTA,
	VGA_COLOR_LIGHT_CYAN,
	VGA_COLOR_WHITE,
	VGA_COLOR_BLACK,
	VGA_COLOR_BLACK
};

static int vga_tty_read(struct tty *tty, void *data, size_t len)
{
	/* XXX */
	(void)tty;
	(void)data;
	(void)len;
	return -EAGAIN;
}

static void scroll_up(void)
{
	for (size_t y = 1; y < g_vga_txt.height; ++y)
	{
		for (size_t x = 0; x < g_vga_txt.width; ++x)
			set_val(x, y - 1, get_val(x, y));
	}
	for (size_t x = 0; x < g_vga_txt.width; ++x)
		set_val(x, g_vga_txt.height - 1, 0);
}

static void putchar(struct vga_txt_tty *vga_tty, char c)
{
	if (c == 0x7F)
	{
		if (vga_tty->column == 0)
		{
			vga_tty->column = g_vga_txt.width - 1;
			if (vga_tty->row)
				vga_tty->row--;
		}
		else
		{
			vga_tty->column--;
		}
		set_cursor(vga_tty->column, vga_tty->row);
		set_char(vga_tty->column, vga_tty->row, ' ', entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
		return;
	}

	if (c == '\b')
	{
		if (vga_tty->column == 0)
		{
			vga_tty->column = g_vga_txt.width - 1;
			if (vga_tty->row)
				vga_tty->row--;
		}
		else
		{
			vga_tty->column--;
		}
		set_cursor(vga_tty->column, vga_tty->row);
		return;
	}

	if (c == '\n')
	{
		if (vga_tty->row == g_vga_txt.height - 1)
			scroll_up();
		else
			vga_tty->row++;
		vga_tty->column = 0;
		set_cursor(vga_tty->column, vga_tty->row);
		set_char(vga_tty->column, vga_tty->row, ' ', entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
		return;
	}

	set_char(vga_tty->column, vga_tty->row, c, vga_tty->color);
	vga_tty->column++;

	if (vga_tty->column == g_vga_txt.width)
	{
		if (vga_tty->row == g_vga_txt.height - 1)
			scroll_up();
		else
			vga_tty->row++;
		vga_tty->column = 0;
	}

	set_cursor(vga_tty->column, vga_tty->row);
	set_char(vga_tty->column, vga_tty->row, ' ', entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
}

static int vga_tty_write(struct tty *tty, const void *data, size_t len)
{
	struct vga_txt_tty *vga_tty = tty->userptr;
	for (size_t i = 0; i < len; ++i)
		putchar(vga_tty, ((char*)data)[i]);
	return len;
}

static void update_color(struct vga_txt_tty *vga_tty)
{
	vga_tty->color = (g_bg_colors[vga_tty->bg_color] << 4)
	                | g_fg_colors[vga_tty->fg_color + vga_tty->bold * 10];
}

static int vga_tty_ctrl(struct tty *tty, enum tty_ctrl ctrl, uint32_t val)
{
	struct vga_txt_tty *vga_tty = tty->userptr;
	switch (ctrl)
	{
		case TTY_CTRL_CM:
		{
			uint8_t line = val >> 8;
			uint8_t column = val;
			set_cursor(column, line);
			break;
		}
		case TTY_CTRL_GC:
			vga_tty->bold = 0;
			vga_tty->fg_color = 9;
			vga_tty->bg_color = 9;
			update_color(vga_tty);
			break;
		case TTY_CTRL_GFG:
			vga_tty->fg_color = val;
			update_color(vga_tty);
			break;
		case TTY_CTRL_GBG:
			vga_tty->bg_color = val;
			update_color(vga_tty);
			break;
		case TTY_CTRL_GB:
			vga_tty->bold = 1;
			update_color(vga_tty);
			break;
		case TTY_CTRL_GRB:
			vga_tty->bold = 0;
			update_color(vga_tty);
			break;
		case TTY_CTRL_PCD:
			disable_cursor(); /* only if current tty */
			break;
		case TTY_CTRL_PCE:
			enable_cursor(); /* only if current tty */
			break;
		default:
			break;
	}
	return 0;
}

static struct tty_op g_tty_vga_op =
{
	.read = vga_tty_read,
	.write = vga_tty_write,
	.ctrl = vga_tty_ctrl,
};

int vga_txt_mktty(const char *name, int id, struct tty **tty)
{
	struct vga_txt_tty *vga_tty = malloc(sizeof(*vga_tty), M_ZERO);
	if (!vga_tty)
		return ENOMEM;
	vga_tty->fg_color = 7;
	update_color(vga_tty);
	int res = tty_create(name, makedev(4, id), &g_tty_vga_op, tty);
	if (res)
		return res;
	(*tty)->userptr = vga_tty;
	return 0;
}
