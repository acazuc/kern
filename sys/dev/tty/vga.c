#include "tty.h"
#include "dev/vga/vga.h"

#include <inttypes.h>
#include <stdio.h>
#include <errno.h>

/* XXX: remove static global */
static uint8_t g_row;
static uint8_t g_col;
static uint8_t g_color = VGA_COLOR_LIGHT_GREY | (VGA_COLOR_BLACK << 4);

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
	for (size_t y = 1; y < VGA_HEIGHT - 1; ++y)
	{
		for (size_t x = 0; x < VGA_WIDTH; ++x)
			vga_set_val(x, y - 1, vga_get_val(x, y));
	}
	for (size_t x = 0; x < VGA_WIDTH; ++x)
		vga_set_val(x, VGA_HEIGHT - 2, 0);
}

static void putchar(char c)
{
	if (g_row == VGA_HEIGHT - 1)
	{
		scroll_up();
		g_row--;
		g_col = 0;
	}

	if (g_col == VGA_WIDTH - 1)
	{
		scroll_up();
		g_col = 0;
	}

	if (c == '\n')
	{
		g_row++;
		g_col = 0;
		return;
	}

	vga_set_char(g_col, g_row, c, g_color);
	g_col++;
}

static int vga_tty_write(struct tty *tty, const void *data, size_t len)
{
	/* XXX */
	(void)tty;
	for (size_t i = 0; i < len; ++i)
		putchar(((char*)data)[i]);
	return len;
}

static uint8_t g_fg_colors[] =
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

static uint8_t g_bg_colors[] =
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

/* XXX: remove static global */
static uint8_t g_fg_color = 9;
static uint8_t g_bg_color = 9;
static uint8_t g_bold;

static void update_color(void)
{
	g_color = (g_bg_colors[g_bg_color] << 4) | g_fg_colors[g_fg_color + g_bold * 10];
}

static int vga_tty_ctrl(struct tty *tty, enum tty_ctrl ctrl, uint32_t val)
{
	(void)tty; /* XXX */
	switch (ctrl)
	{
		case TTY_CTRL_CM:
		{
			uint8_t line = val >> 8;
			uint8_t column = val;
			vga_set_cursor(column, line);
			break;
		}
		case TTY_CTRL_GC:
			g_bold = 0;
			g_fg_color = 9;
			g_bg_color = 9;
			update_color();
			break;
		case TTY_CTRL_GFG:
			g_fg_color = val;
			update_color();
			break;
		case TTY_CTRL_GBG:
			g_bg_color = val;
			update_color();
			break;
		case TTY_CTRL_GB:
			g_bold = 1;
			update_color();
			break;
		case TTY_CTRL_GRB:
			g_bold = 0;
			update_color();
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

int tty_create_vga(const char *name, struct tty **tty)
{
	return tty_create(name, &g_tty_vga_op, tty);
}
