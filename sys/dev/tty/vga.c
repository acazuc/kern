#include "tty.h"
#include "dev/vga/vga.h"

#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

struct vga_tty
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
	for (size_t y = 1; y < VGA_HEIGHT; ++y)
	{
		for (size_t x = 0; x < VGA_WIDTH; ++x)
			vga_set_val(x, y - 1, vga_get_val(x, y));
	}
	for (size_t x = 0; x < VGA_WIDTH; ++x)
		vga_set_val(x, VGA_HEIGHT - 1, 0);
}

static void putchar(struct vga_tty *vga_tty, char c)
{
	if (c == 0x7F)
	{
		if (vga_tty->column == 0)
		{
			vga_tty->column = VGA_WIDTH - 1;
			if (vga_tty->row)
				vga_tty->row--;
		}
		else
		{
			vga_tty->column--;
		}
		vga_set_cursor(vga_tty->column, vga_tty->row);
		vga_set_char(vga_tty->column, vga_tty->row, ' ', vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
		return;
	}

	if (c == '\b')
	{
		if (vga_tty->column == 0)
		{
			vga_tty->column = VGA_WIDTH - 1;
			if (vga_tty->row)
				vga_tty->row--;
		}
		else
		{
			vga_tty->column--;
		}
		vga_set_cursor(vga_tty->column, vga_tty->row);
		return;
	}

	if (c == '\n')
	{
		if (vga_tty->row == VGA_HEIGHT - 1)
			scroll_up();
		else
			vga_tty->row++;
		vga_tty->column = 0;
		vga_set_cursor(vga_tty->column, vga_tty->row);
		vga_set_char(vga_tty->column, vga_tty->row, ' ', vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
		return;
	}

	vga_set_char(vga_tty->column, vga_tty->row, c, vga_tty->color);
	vga_tty->column++;

	if (vga_tty->column == VGA_WIDTH)
	{
		if (vga_tty->row == VGA_HEIGHT - 1)
			scroll_up();
		else
			vga_tty->row++;
		vga_tty->column = 0;
	}

	vga_set_cursor(vga_tty->column, vga_tty->row);
	vga_set_char(vga_tty->column, vga_tty->row, ' ', vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
}

static int vga_tty_write(struct tty *tty, const void *data, size_t len)
{
	struct vga_tty *vga_tty = tty->userptr;
	for (size_t i = 0; i < len; ++i)
		putchar(vga_tty, ((char*)data)[i]);
	return len;
}

static void update_color(struct vga_tty *vga_tty)
{
	vga_tty->color = (g_bg_colors[vga_tty->bg_color] << 4)
	                | g_fg_colors[vga_tty->fg_color + vga_tty->bold * 10];
}

static int vga_tty_ctrl(struct tty *tty, enum tty_ctrl ctrl, uint32_t val)
{
	struct vga_tty *vga_tty = tty->userptr;
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
			vga_disable_cursor(); /* only if current tty */
			break;
		case TTY_CTRL_PCE:
			vga_enable_cursor(); /* only if current tty */
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

int tty_create_vga(const char *name, int id, struct tty **tty)
{
	struct vga_tty *vga_tty = malloc(sizeof(*vga_tty), M_ZERO);
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
