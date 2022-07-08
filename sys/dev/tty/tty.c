#include "tty.h"
#include "fs/devfs/devfs.h"
#include <sys/file.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

struct tty *curtty;

static int write_char(struct tty *tty, uint8_t c);

int tty_fopen(struct file *file, struct fs_node *node)
{
	/* XXX */
	(void)file;
	(void)node;
	return 0;
}

int tty_fclose(struct file *file)
{
	/* XXX */
	(void)file;
	return 0;
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

static const struct file_op g_tty_fop =
{
	.open = tty_fopen,
	.close = tty_fclose,
	.write = tty_fwrite,
	.read = tty_fread,
	.ioctl = tty_fioctl,
};

int tty_create(const char *name, dev_t rdev, struct tty_op *op, struct tty **tty)
{
	*tty = malloc(sizeof(**tty), M_ZERO);
	if (!*tty)
		return ENOMEM;
	(*tty)->op = op;
	(*tty)->rbuf_size = 0;
	(*tty)->wbuf_size = 0;
	(*tty)->ctrl_state = 0;
	(*tty)->termios.c_iflag = IXON;
	(*tty)->termios.c_oflag = OPOST;
	(*tty)->termios.c_lflag = ISIG | ICANON;
	(*tty)->termios.c_cc[VEOF] = 4; /* EOT, ctrl+D */
	(*tty)->termios.c_cc[VEOL] = 0; /* NUL */
	(*tty)->termios.c_cc[VERASE] = 8; /* \b, ctrl+H */
	(*tty)->termios.c_cc[VINTR] = 3; /* ETX, ctrl+C */
	(*tty)->termios.c_cc[VKILL] = 21; /* NAK, ctrl+U */
	(*tty)->termios.c_cc[VMIN] = 0;
	(*tty)->termios.c_cc[VQUIT] = 28; /* FS, ctrl+\ */
	(*tty)->termios.c_cc[VSTART] = 17; /* DC1, ctrl+Q */
	(*tty)->termios.c_cc[VSTOP] = 19; /* DC3, ctrl+S */
	(*tty)->termios.c_cc[VSUSP] = 26; /* SUB, ctrl+Z */
	(*tty)->termios.c_cc[VTIME] = 0;
	for (size_t i = 0; i < sizeof((*tty)->args) / sizeof(*(*tty)->args); ++i)
		(*tty)->args[i] = 0;
	(*tty)->args_nb = 0;
	int res = devfs_mkcdev(name, 0, 0, 0600, rdev, &g_tty_fop, &(*tty)->cdev);
	if (res)
	{
		free(*tty);
		return ENOMEM;
	}
	(*tty)->cdev->node->userptr = *tty;
	return 0;
}

static int input_c(struct tty *tty, uint8_t c)
{
	if (tty->termios.c_lflag & ISIG)
	{
		if (c == tty->termios.c_cc[VINTR])
		{
			/* XXX SIGINT */
			return 1;
		}
		if (c == tty->termios.c_cc[VQUIT])
		{
			/* XXX SIGQUIT */
			return 1;
		}
		if (c == tty->termios.c_cc[VSUSP])
		{
			/* XXX SIGTSTP */
			return 1;
		}
	}
	if (tty->termios.c_iflag & IXON)
	{
		if (c == tty->termios.c_cc[VSTART])
		{
			tty->flags |= TTY_STOPPED;
			return 1;
		}
		if (c == tty->termios.c_cc[VSTOP])
		{
			tty->flags &= ~TTY_STOPPED;
			return 1;
		}
	}
	if (tty->termios.c_lflag & ICANON)
	{
		if (c == tty->termios.c_cc[VEOF])
		{
			tty->flags |= TTY_EOF;
			memcpy(tty->rbuf, tty->line, tty->line_size);
			tty->rbuf_size = tty->line_size;
			tty->line_size = 0;
			/* XXX: update cursor */
			write_char(tty, c);
			return 1;
		}
		if (c == tty->termios.c_cc[VEOL] || c == '\n')
		{
			/* XXX handle non-empty rbuf */
			if (tty->line_size == sizeof(tty->line))
				tty->line[sizeof(tty->line) - 1] = c;
			else
				tty->line[tty->line_size++] = c;
			memcpy(tty->rbuf, tty->line, tty->line_size);
			tty->rbuf_size = tty->line_size;
			tty->line_size = 0;
			write_char(tty, c);
			/* XXX: update cursor */
			return 1;
		}
		if (c == tty->termios.c_cc[VERASE])
		{
			if (!tty->line_size)
				return 1;
			tty->line_size--;
			write_char(tty, '\x7F');
			return 1;
		}
		if (c == tty->termios.c_cc[VKILL])
		{
			/* XXX */
			return 1;
		}
		if (tty->line_size == sizeof(tty->line))
			return 0;
		tty->line[tty->line_size++] = c;
		/* XXX: update cursor */
		write_char(tty, c);
		return 1;
	}
	if (tty->rbuf_size == sizeof(tty->rbuf))
		return 0;
	if (tty->termios.c_iflag & ISTRIP)
		c &= 0x7F;
	if ((tty->termios.c_iflag & IGNCR) && c == '\r')
		return 1;
	if ((tty->termios.c_iflag & INLCR) && c == '\n')
		c = '\r';
	tty->rbuf[tty->rbuf_size++] = c;
	return 1;
}

int tty_input(struct tty *tty, const void *data, size_t count)
{
	for (size_t i = 0; i < count; ++i)
	{
		if (!input_c(tty, ((char*)data)[i]))
			return i;
	}
	return count;
}

static int write_char(struct tty *tty, uint8_t c)
{
	if (!(tty->termios.c_oflag & OPOST))
		return tty->op->write(tty, &c, 1);
	if (c == '\n')
		return tty->op->write(tty, &c, 1);
	if (c == '\r')
	{
		if (!(tty->termios.c_oflag & ONLRET))
			return 1;
		if (tty->termios.c_oflag & ONOCR)
		{
			/* XXX */
		}
		if (tty->termios.c_oflag & OCRNL)
			c = '\n';
		return tty->op->write(tty, &c, 1);
	}
	if (c < 32)
	{
		static const char tmp = '^';
		int res = tty->op->write(tty, &tmp, 1);
		if (res < 0)
			return res;
		c += 64;
		return tty->op->write(tty, &c, 1);
	}
	return tty->op->write(tty, &c, 1);
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
					write_char(tty, s[i]);
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
					default:
						/* unknown */
						break;
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
								case 90:
								case 91:
								case 92:
								case 93:
								case 94:
								case 95:
								case 96:
								case 97:
									tty->op->ctrl(tty, TTY_CTRL_GFGB, tty->args[n] - 90);
									break;
								case 100:
								case 101:
								case 102:
								case 103:
								case 104:
								case 105:
								case 106:
								case 107:
									tty->op->ctrl(tty, TTY_CTRL_GBGB, tty->args[n] - 100);
									break;
								default:
									/* unknown */
									break;
							}
						}
						break;
					case '=':
						tty->ctrl_state = 3;
						continue;
					case '?':
						tty->ctrl_state = 4;
						continue;
					default:
						/* unknown */
						break;
				}
				tty->args[0] = 0;
				tty->args_nb = 0;
				tty->ctrl_state = 0;
				break;
			case 3: /* after ESC[= */
				if (isdigit(s[i]))
				{
					if (tty->args_nb < sizeof(tty->args) / sizeof(*tty->args))
						tty->args[tty->args_nb] = tty->args[tty->args_nb] * 10 + (s[i] - '0');
					continue;
				}
				tty->args_nb++;
				switch (s[i])
				{
					case 'h':
						switch (tty->args[0])
						{
							case 0:
								tty->op->ctrl(tty, TTY_CTRL_S0, 0);
								break;
							case 1:
								tty->op->ctrl(tty, TTY_CTRL_S1, 0);
								break;
							case 2:
								tty->op->ctrl(tty, TTY_CTRL_S2, 0);
								break;
							case 3:
								tty->op->ctrl(tty, TTY_CTRL_S3, 0);
								break;
							case 4:
								tty->op->ctrl(tty, TTY_CTRL_S4, 0);
								break;
							case 5:
								tty->op->ctrl(tty, TTY_CTRL_S5, 0);
								break;
							case 6:
								tty->op->ctrl(tty, TTY_CTRL_S6, 0);
								break;
							case 7:
								tty->op->ctrl(tty, TTY_CTRL_SLW, 0);
								break;
							case 13:
								tty->op->ctrl(tty, TTY_CTRL_S13, 0);
								break;
							case 14:
								tty->op->ctrl(tty, TTY_CTRL_S14, 0);
								break;
							case 15:
								tty->op->ctrl(tty, TTY_CTRL_S15, 0);
								break;
							case 16:
								tty->op->ctrl(tty, TTY_CTRL_S16, 0);
								break;
							case 17:
								tty->op->ctrl(tty, TTY_CTRL_S17, 0);
								break;
							case 18:
								tty->op->ctrl(tty, TTY_CTRL_S18, 0);
								break;
							case 19:
								tty->op->ctrl(tty, TTY_CTRL_S19, 0);
								break;
							default:
								/* unknown */
								break;
						}
						break;
					default:
						/* unknown */
						break;
				}
				tty->args[0] = 0;
				tty->args_nb = 0;
				tty->ctrl_state = 0;
				break;
			case 4: /* after ESC[? */
				if (isdigit(s[i]))
				{
					if (tty->args_nb < sizeof(tty->args) / sizeof(*tty->args))
						tty->args[tty->args_nb] = tty->args[tty->args_nb] * 10 + (s[i] - '0');
					continue;
				}
				tty->args_nb++;
				switch (s[i])
				{
					case 'l':
						switch (tty->args[0])
						{
							case 25:
								tty->op->ctrl(tty, TTY_CTRL_PCD, 0);
								break;
							case 47:
								tty->op->ctrl(tty, TTY_CTRL_PSR, 0);
								break;
							default:
									/* unknown */
								break;
						}
						break;
					case 'h':
						switch (tty->args[0])
						{
							case 25:
								tty->op->ctrl(tty, TTY_CTRL_PCE, 0);
								break;
							case 47:
								tty->op->ctrl(tty, TTY_CTRL_PSS, 0);
								break;
							default:
									/* unknown */
								break;
						}
						break;
					default:
						/* unknown */
						break;
				}
				break;
		}
	}
	return count;
}
