#ifndef DEV_TTY_H
#define DEV_TTY_H

#include <stdint.h>
#include <stddef.h>

struct fs_cdev;
struct tty_op;

struct tty
{
	struct fs_cdev *cdev;
	struct tty_op *op;
	char wbuf[256]; /* XXX: less than 256 ? */
	char rbuf[256]; /* XXX: resize ? */
	size_t wbuf_size;
	size_t rbuf_size;
	void *userptr;
};

enum tty_ctrl
{
	TTY_BEL,
	TTY_BS,
	TTY_HT,
	TTY_LF,
	TTY_VT,
	TTY_FF,
	TTY_CR,
	TTY_ESC,
	TTY_DEL,
	TTY_CURSOR_HOME,
	TTY_CURSOR_MOVE,
	TTY_CURSOR_MOVE_UP,
	TTY_CURSOR_MOVE_DOWN,
	TTY_CURSOR_MOVE_RIGHT,
	TTY_CURSOR_MOVE_LEFT,
	TTY_CURSOR_MOVE_BEG_NL,
	TTY_CURSOR_MOVE_BEG_PL,
	TTY_CURSOR_MOVE_COLUMN,
	TTY_CURSOR_GETPOS,
	TTY_CURSOR_LINEUP,
	TTY_CURSOR_SAVEPOS,
	TTY_CURSOR_RESTOREPOS,
	TTY_ERASE_AFTERCUR,
	TTY_ERASE_BEFORECUR,
	TTY_ERASE_SCREEN,
	TTY_ERASE_SAVEDLINE,
	TTY_ERASE_LINEAFTER,
	TTY_ERASE_LINEBEFORE,
	TTY_ERASE_LINE,
	TTY_GRAPH_BOLD,
	TTY_GRAPH_DIM,
	TTY_GRAPH_ITALIC,
	TTY_GRAPH_UNDERLINE,
	TTY_GRAPH_BLINK,
	TTY_GRAPH_REVERSE,
	TTY_GRAPH_HIDDEN,
	TTY_GRAPH_STRIKE,
	TTY_GRAPH_FG,
	TTY_GRAPH_BG,
	TTY_GRAPH_FG24,
	TTY_GRAPH_BG24,
	TTY_SCREEN_40X25_MONO,
	TTY_SCREEN_40X25_COLOR,
	TTY_SCREEN_80X25_MONO,
	TTY_SCREEN_80X25_COLOR,
	TTY_SCREEN_320X200_4COL,
	TTY_SCREEN_320X200_MONO,
	TTY_SCREEN_640X200_MONO,
	TTY_SCREEN_LINEWRAP,
	TTY_SCREEN_320X200_COLOR,
	TTY_SCREEN_640X200_COLOR16,
	TTY_SCREEN_640X350_MONO2,
	TTY_SCREEN_640X350_COLOR16,
	TTY_SCREEN_640X480_MONO2,
	TTY_SCREEN_640X480_COLOR16,
	TTY_SCREEN_320X200_COLOR256,
	TTY_SCREEN_RESET,
	TTY_PRIV_CURINV,
	TTY_PRIV_CURVIS,
	TTY_PRIV_RESSCREEN,
	TTY_PRIV_SAVESCREEN,
	TTY_PRIV_ENAALTBUF,
	TTY_PRIV_DISALTBUF,
};

struct tty_op
{
	int (*read)(struct tty *tty, void *data, size_t len, size_t *read);
	int (*write)(struct tty *tty, const void *data, size_t len, size_t *written);
	int (*ctrl)(struct tty *tty, enum tty_ctrl type, uint32_t len);
};

int tty_create(const char *name, struct tty_op *op, struct tty **tty);
int tty_create_vga(const char *name, struct tty **tty);
int tty_input(struct tty *tty, const void *data, size_t count);

extern struct tty *curtty;

#endif
