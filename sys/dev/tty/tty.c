#include "tty.h"
#include "fs/devfs/devfs.h"
#include <sys/file.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

struct tty *curtty;

int tty_fopen(struct file *file)
{
	/* XXX */
	(void)file;
	return EINVAL;
}

int tty_fclose(struct file *file)
{
	/* XXX */
	(void)file;
	return EINVAL;
}

int tty_fwrite(struct file *file, const void *data, size_t count)
{
	if (!file->node || !file->node->userptr)
		return -EINVAL; /* XXX */
	struct tty *tty = file->node->userptr;
	return tty_write(tty, data, count);
}

int tty_fread(struct file *file, void *data, size_t count)
{
	if (!file->node || !file->node->userptr)
		return -EINVAL; /* XXX */
	struct tty *tty = file->node->userptr;
	if (!tty || !tty->op)
		return -EINVAL; /* XXX */
	if (count > tty->rbuf_size)
		count = tty->rbuf_size;
	if (!count)
		return -EAGAIN;
	memcpy(data, tty->rbuf, count);
	memcpy(tty->rbuf, &tty->rbuf[count], tty->rbuf_size - count);
	tty->rbuf_size -= count;
	return count;
}

int tty_fioctl(struct file *file, unsigned long req, intptr_t data)
{
	if (!file->node || !file->node->userptr)
		return -EINVAL; /* XXX */
	struct tty *tty = file->node->userptr;
	if (!tty || !tty->op)
		return -EINVAL; /* XXX */
	/* XXX */
	(void)req;
	(void)data;
	return -ENOTTY;
}

struct file_op g_tty_fop =
{
	.open = tty_fopen,
	.close = tty_fclose,
	.write = tty_fwrite,
	.read = tty_fread,
	.ioctl = tty_fioctl,
};

int tty_create(const char *name, struct tty_op *op, struct tty **tty)
{
	*tty = malloc(sizeof(**tty), 0);
	if (!*tty)
		return ENOMEM;
	(*tty)->op = op;
	(*tty)->rbuf_size = 0;
	(*tty)->wbuf_size = 0;
	(*tty)->ctrl_state = 0;
	for (size_t i = 0; i < sizeof((*tty)->args) / sizeof(*(*tty)->args); ++i)
		(*tty)->args[i] = 0;
	(*tty)->args_nb = 0;
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

int tty_write(struct tty *tty, const void *data, size_t count)
{
	if (!tty || !tty->op)
		return -EINVAL; /* XXX */
	const char *s = data;
	for (size_t i = 0; i < count; ++i)
	{
		switch (tty->ctrl_state)
		{
			case 0: /* no state */
				if (s[i] != '\033')
				{
					tty->op->write(tty, &s[i], 1);
					continue;
				}
				tty->ctrl_state = 1;
				break;
			case 1: /* after ESC */
				switch (s[i])
				{
					case '[':
						tty->ctrl_state = 2;
						continue;
					case 'M':
						tty->op->ctrl(tty, TTY_CTRL_CLU, 0);
						tty->ctrl_state = 0;
						continue;
					case '7':
						tty->op->ctrl(tty, TTY_CTRL_CSP, 0);
						tty->ctrl_state = 0;
						continue;
					case '8':
						tty->op->ctrl(tty, TTY_CTRL_CRP, 0);
						tty->ctrl_state = 0;
						continue;
				}
				break;
			case 2: /* after ESC[ */
				if (isdigit(s[i]))
				{
					if (tty->args_nb < sizeof(tty->args) / sizeof(*tty->args))
						tty->args[tty->args_nb] = tty->args[tty->args_nb] * 10 + (s[i] - '0');
					continue;
				}
				tty->args_nb++;
				switch (s[i])
				{
					case ';':
						if (tty->args_nb < sizeof(tty->args) / sizeof(*tty->args))
							tty->args[tty->args_nb] = 0;
						continue;
					case 'A':
						tty->op->ctrl(tty, TTY_CTRL_CU, tty->args[0]);
						break;
					case 'B':
						tty->op->ctrl(tty, TTY_CTRL_CD, tty->args[0]);
						break;
					case 'C':
						tty->op->ctrl(tty, TTY_CTRL_CR, tty->args[0]);
						break;
					case 'D':
						tty->op->ctrl(tty, TTY_CTRL_CL, tty->args[0]);
						break;
					case 'E':
						tty->op->ctrl(tty, TTY_CTRL_CDL, tty->args[0]);
						break;
					case 'F':
						tty->op->ctrl(tty, TTY_CTRL_CUL, tty->args[0]);
						break;
					case 'G':
						tty->op->ctrl(tty, TTY_CTRL_CC, tty->args[0]);
						break;
					case 'H':
					case 'f':
					{
						uint8_t line;
						uint8_t column;
						if (tty->args_nb == 1)
						{
							line = 0;
							column = 0;
						}
						else if (tty->args_nb == 2)
						{
							line = tty->args[0];
							column = tty->args[1];
						}
						else
						{
							break;
						}
						tty->op->ctrl(tty, TTY_CTRL_CM, (line << 8) | column);
						break;
					}
					case 'J':
						switch (tty->args[0])
						{
							case 0:
								tty->op->ctrl(tty, TTY_CTRL_EAC, 0);
								break;
							case 1:
								tty->op->ctrl(tty, TTY_CTRL_EBC, 0);
								break;
							case 2:
								tty->op->ctrl(tty, TTY_CTRL_ES, 0);
								break;
							case 3:
								tty->op->ctrl(tty, TTY_CTRL_ESL, 0);
								break;
							default:
								/* unknown */
								break;
						}
						break;
					case 'K':
						switch (tty->args[0])
						{
							case 0:
								tty->op->ctrl(tty, TTY_CTRL_ELA, 0);
								break;
							case 1:
								tty->op->ctrl(tty, TTY_CTRL_ELB, 0);
								break;
							case 2:
								tty->op->ctrl(tty, TTY_CTRL_EL, 0);
								break;
							default:
								/* unknown */
								break;
						}
						break;
					case 'm':
						for (int n = 0; n < tty->args_nb; ++n)
						{
							switch (tty->args[n])
							{
								case 0:
									tty->op->ctrl(tty, TTY_CTRL_GC, 0);
									break;
								case 1:
									tty->op->ctrl(tty, TTY_CTRL_GB, 0);
									break;
								case 2:
									tty->op->ctrl(tty, TTY_CTRL_GD, 0);
									break;
								case 3:
									tty->op->ctrl(tty, TTY_CTRL_GI, 0);
									break;
								case 4:
									tty->op->ctrl(tty, TTY_CTRL_GU, 0);
									break;
								case 5:
									tty->op->ctrl(tty, TTY_CTRL_GBL, 0);
									break;
								case 7:
									tty->op->ctrl(tty, TTY_CTRL_GR, 0);
									break;
								case 8:
									tty->op->ctrl(tty, TTY_CTRL_GH, 0);
									break;
								case 9:
									tty->op->ctrl(tty, TTY_CTRL_GS, 0);
									break;
								case 22:
									tty->op->ctrl(tty, TTY_CTRL_GRB, 0);
									break;
								case 23:
									tty->op->ctrl(tty, TTY_CTRL_GRD, 0);
									break;
								case 24:
									tty->op->ctrl(tty, TTY_CTRL_GRU, 0);
									break;
								case 25:
									tty->op->ctrl(tty, TTY_CTRL_GRBL, 0);
									break;
								case 27:
									tty->op->ctrl(tty, TTY_CTRL_GRR, 0);
									break;
								case 28:
									tty->op->ctrl(tty, TTY_CTRL_GRH, 0);
									break;
								case 29:
									tty->op->ctrl(tty, TTY_CTRL_GRS, 0);
									break;
								case 30:
								case 31:
								case 32:
								case 33:
								case 34:
								case 35:
								case 36:
								case 37:
								case 39:
									tty->op->ctrl(tty, TTY_CTRL_GFG, tty->args[n] - 30);
									break;
								case 38:
									n++;
									if (n >= tty->args_nb)
										break;
									switch (tty->args[n])
									{
										case 5:
											n++;
											if (n >= tty->args_nb)
												break;
											tty->op->ctrl(tty, TTY_CTRL_GFG256, tty->args[n]);
											break;
										case 2:
										{
											uint8_t r;
											uint8_t g;
											uint8_t b;
											n++;
											if (n >= tty->args_nb)
												break;
											r = tty->args[n];
											n++;
											if (n >= tty->args_nb)
												break;
											g = tty->args[n];
											n++;
											if (n >= tty->args_nb)
												break;
											b = tty->args[n];
											tty->op->ctrl(tty, TTY_CTRL_GFG24, (r << 16) | (g << 8) | b);
											break;
										}
									}
									break;
								case 40:
								case 41:
								case 42:
								case 43:
								case 44:
								case 45:
								case 46:
								case 47:
								case 49:
									tty->op->ctrl(tty, TTY_CTRL_GBG, tty->args[n] - 40);
									break;
								case 48:
									n++;
									if (n >= tty->args_nb)
										break;
									switch (tty->args[n])
									{
										case 5:
											n++;
											if (n >= tty->args_nb)
												break;
											tty->op->ctrl(tty, TTY_CTRL_GBG256, tty->args[n]);
											break;
										case 2:
										{
											uint8_t r;
											uint8_t g;
											uint8_t b;
											n++;
											if (n >= tty->args_nb)
												break;
											r = tty->args[n];
											n++;
											if (n >= tty->args_nb)
												break;
											g = tty->args[n];
											n++;
											if (n >= tty->args_nb)
												break;
											b = tty->args[n];
											tty->op->ctrl(tty, TTY_CTRL_GBG24, (r << 16) | (g << 8) | b);
											break;
										}
									}
									break;
							}
						}
						break;
					default:
						/* unknown */
						break;
				}
				tty->args[0] = 0;
				tty->args_nb = 0;
				tty->ctrl_state = 0;
				continue;
		}
	}
	return count;
}
