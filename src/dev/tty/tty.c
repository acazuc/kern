#include "tty.h"
#include "fs/devfs/devfs.h"
#include <sys/file.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

struct tty *curtty;

static int tty_fopen(struct file *file)
{
	return EINVAL;
}

static int tty_fclose(struct file *file)
{
	return EINVAL;
}

static int tty_fwrite(struct file *file, const void *data, size_t count, size_t *written)
{
	if (!file->node || !file->node->userptr)
		return EINVAL; /* XXX */
	struct tty *tty = file->node->userptr;
	if (!tty || !tty->op)
		return EINVAL; /* XXX */
	return tty->op->write(tty, data, count, written);
}

static int tty_fread(struct file *file, void *data, size_t count, size_t *read)
{
	if (!file->node || !file->node->userptr)
		return EINVAL; /* XXX */
	struct tty *tty = file->node->userptr;
	if (!tty || !tty->op)
		return EINVAL; /* XXX */
	if (count > tty->rbuf_size)
		count = tty->rbuf_size;
	if (!count)
	{
		*read = 0;
		return 0;
	}
	memcpy(data, tty->rbuf, count);
	memcpy(tty->rbuf, &tty->rbuf[count], tty->rbuf_size - count);
	tty->rbuf_size -= count;
	*read = count;
	return 0;
}

struct file_op g_tty_fop =
{
	.open = tty_fopen,
	.close = tty_fclose,
	.write = tty_fwrite,
	.read = tty_fread,
};

int tty_create(const char *name, struct tty_op *op, struct tty **tty)
{
	*tty = malloc(sizeof(**tty), 0);
	if (!*tty)
		return ENOMEM;
	(*tty)->op = op;
	(*tty)->wbuf_size = 0;
	int res = devfs_mkcdev(name, 0, 0, 0600, &g_tty_fop, &(*tty)->cdev);
	if (res)
	{
		free(*tty);
		return ENOMEM;
	}
	(*tty)->cdev->node->userptr = *tty;
	return 0;
}

int tty_input(struct tty *tty, const void *data, size_t count)
{
	if (count > sizeof(tty->rbuf) - tty->rbuf_size)
		count = sizeof(tty->rbuf) - tty->rbuf_size;
	memcpy(&tty->rbuf[tty->rbuf_size], data, count);
	tty->rbuf_size += count;
	return EINVAL;
}
