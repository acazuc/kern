#include "vga.h"

#include <sys/vmm.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <arch.h>
#include <tty.h>

extern char font8x8_basic[128][8];

struct vga_rgb
{
	struct vga vga;
	uint8_t *addr;
	size_t size;
};

struct color
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

static struct vga_rgb g_vga_rgb;

struct vga_rgb_tty
{
	size_t x;
	size_t y;
	int bold;
	struct color fg_color;
	struct color bg_color;
};

const struct color g_colors[256] =
{
	{0  , 0  , 0  }, /* black */
	{170, 0  , 0  }, /* red */
	{0  , 170, 0  }, /* green */
	{170, 85 , 0  }, /* brown */
	{0  , 0  , 170}, /* blue */
	{170, 0  , 170}, /* magenta */
	{0  , 170, 170}, /* cyan */
	{170, 170, 170}, /* light grey */
	{170, 170, 170}, /* light grey */
	{170, 170, 170}, /* light grey */
	{85 , 85 , 85 }, /* dark grey */
	{255, 85 , 85 }, /* light red */
	{85 , 255, 85 }, /* light green */
	{255, 255, 85 }, /* light brown */
	{85 , 85 , 255}, /* light blue */
	{255, 85 , 255}, /* light magenta */
	{85 , 255, 255}, /* light cyan */
	{255, 255, 255}, /* white */
	{255, 255, 255}, /* white */
	{255, 255, 255}, /* white */
};

static void set_pixel(struct vga_rgb *vga, size_t x, size_t y, struct color color)
{
	/* XXX handle bpp != 32 */
	uint8_t *dst = &vga->addr[y * vga->vga.pitch + x * vga->vga.bpp / 8];
	dst[0] = color.b;
	dst[1] = color.g;
	dst[2] = color.r;
}

static void set_char(struct vga_rgb *vga, size_t x, size_t y, uint8_t c, struct color fg, struct color bg)
{
	if (c > 127)
		c = 0;
	for (size_t i = 0; i < 8; ++i)
	{
		for (size_t j = 0; j < 8; ++j)
		{
			set_pixel(vga, x + i, y + j, (font8x8_basic[c][j] & (1 << i)) ? fg : bg);
		}
	}
}

static void scroll_up(void)
{
	for (size_t y = 0; y < g_vga_rgb.vga.height - 8; y += 8)
		memcpy(&g_vga_rgb.addr[y * g_vga_rgb.vga.pitch], &g_vga_rgb.addr[(y + 8) * g_vga_rgb.vga.pitch], g_vga_rgb.vga.pitch * 8);
	memset(&g_vga_rgb.addr[(g_vga_rgb.vga.height - 8) * g_vga_rgb.vga.pitch], 0, g_vga_rgb.vga.pitch * 8);
}

void vga_rgb_init(uint32_t paddr, uint32_t width, uint32_t height, uint32_t pitch, uint32_t bpp)
{
	g_vga_rgb.vga.rgb = 1;
	g_vga_rgb.vga.paddr = paddr;
	g_vga_rgb.vga.width = width;
	g_vga_rgb.vga.height = height;
	g_vga_rgb.vga.pitch = pitch;
	g_vga_rgb.vga.bpp = bpp;
	g_vga_rgb.size = height * pitch;
	g_vga_rgb.size += PAGE_SIZE - 1;
	g_vga_rgb.size -= g_vga_rgb.size % PAGE_SIZE;
	g_vga_rgb.addr = vmap(paddr, g_vga_rgb.size);
	assert(g_vga_rgb.addr, "failed to vmap vga rgb\n");
	g_vga = &g_vga_rgb.vga;
}

static int vga_tty_read(struct tty *tty, void *data, size_t len)
{
	/* XXX */
	(void)tty;
	(void)data;
	(void)len;
	return -EAGAIN;
}

static void putchar(struct vga_rgb_tty *vga_tty, uint8_t c)
{
	if (c == 0x7F)
	{
		if (vga_tty->x == 0)
		{
			vga_tty->x = g_vga_rgb.vga.width / 8 - 1;
			if (vga_tty->y)
				vga_tty->y--;
		}
		else
		{
			vga_tty->x--;
		}
		//set_cursor(vga_tty->x, vga_tty->y);
		set_char(&g_vga_rgb, vga_tty->x, vga_tty->y, ' ', g_colors[7], g_colors[0]);
		return;
	}

	if (c == '\b')
	{
		if (vga_tty->x == 0)
		{
			vga_tty->x = g_vga_rgb.vga.width / 8 - 1;
			if (vga_tty->y)
				vga_tty->y--;
		}
		else
		{
			vga_tty->x--;
		}
		//set_cursor(vga_tty->x, vga_tty->y);
		return;
	}

	if (c == '\n')
	{
		if (vga_tty->y >= g_vga_rgb.vga.height / 8)
			scroll_up();
		else
			vga_tty->y++;
		vga_tty->x = 0;
		//set_cursor(vga_tty->x, vga_tty->y);
		//set_char(vga_tty->x, vga_tty->y, ' ', entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
		return;
	}

	set_char(&g_vga_rgb, vga_tty->x * 8, vga_tty->y * 8, c, vga_tty->fg_color, vga_tty->bg_color);
	vga_tty->x++;
	if (vga_tty->x >= g_vga_rgb.vga.width / 8)
	{
		vga_tty->x = 0;
		if (vga_tty->y >= g_vga_rgb.vga.height / 8)
			scroll_up();
		else
			vga_tty->y++;
	}

	//set_cursor(vga_tty->x, vga_tty->y);
}

static int vga_tty_write(struct tty *tty, const void *data, size_t len)
{
	struct vga_rgb_tty *vga_tty = tty->userptr;
	for (size_t i = 0; i < len; ++i)
		putchar(vga_tty, ((uint8_t*)data)[i]);
	return len;
}

static void update_color(struct vga_rgb_tty *vga_tty)
{
	/* XXX */
}

static int vga_tty_ctrl(struct tty *tty, enum tty_ctrl ctrl, uint32_t val)
{
	struct vga_rgb_tty *vga_tty = tty->userptr;
	switch (ctrl)
	{
		case TTY_CTRL_GC:
			vga_tty->bold = 0;
			vga_tty->fg_color = g_colors[7];
			vga_tty->bg_color = g_colors[0];
			update_color(vga_tty);
			break;
		case TTY_CTRL_GFG:
			vga_tty->fg_color = g_colors[val];
			update_color(vga_tty);
			break;
		case TTY_CTRL_GBG:
			vga_tty->bg_color = g_colors[val];
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
		default:
			break;
	}
	return 0;
}

static struct tty_op g_tty_op =
{
	.read = vga_tty_read,
	.write = vga_tty_write,
	.ctrl = vga_tty_ctrl,
};

int vga_rgb_mktty(const char *name, int id, struct tty **tty)
{
	struct vga_rgb_tty *vga_tty = malloc(sizeof(*vga_tty), M_ZERO);
	if (!vga_tty)
		return ENOMEM;
	int res = tty_create(name, makedev(4, id), &g_tty_op, tty);
	if (res)
		return res;
	(*tty)->userptr = vga_tty;
	vga_tty->fg_color = g_colors[7];
	vga_tty->bg_color = g_colors[0];
	return 0;
}
