#include "tty.h"

#include <errno.h>

static int vga_tty_read(struct tty *tty, void *data, size_t len, size_t *read)
{
	return EINVAL;
}

static int vga_tty_write(struct tty *tty, const void *data, size_t len, size_t *written)
{
	shell_putdata(data, len);
	*written = len;
	return 0;
}

static int vga_tty_ctrl(struct tty *tty, enum tty_ctrl type, uint32_t len)
{
	return EINVAL;
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
