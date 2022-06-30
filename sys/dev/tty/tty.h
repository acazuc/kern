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
	uint8_t args[8]; /* escape codes args; XXX: resize ? */
	uint8_t args_nb;
	char wbuf[256]; /* XXX: less than 256 ? */
	char rbuf[256]; /* XXX: resize ? */
	size_t wbuf_size;
	size_t rbuf_size;
	size_t ctrl_state;
	void *userptr;
};

enum tty_ctrl
{
	TTY_CTRL_CM, /* cursor move */
	TTY_CTRL_CU, /* cursor up */
	TTY_CTRL_CD, /* cursor down */
	TTY_CTRL_CR, /* cursor right */
	TTY_CTRL_CL, /* cursor left */
	TTY_CTRL_CDL, /* cursor down line start */
	TTY_CTRL_CUL, /* cursor up line start */
	TTY_CTRL_CC, /* cursor column */
	TTY_CTRL_CGP, /* cursor get pos */
	TTY_CTRL_CLU, /* cursor line up */
	TTY_CTRL_CSP, /* cursor save pos */
	TTY_CTRL_CRP, /* cursor restore pos */
	TTY_CTRL_EAC, /* erase after cursor */
	TTY_CTRL_EBC, /* erase before cursor */
	TTY_CTRL_ES, /* erase screen */
	TTY_CTRL_ESL, /* erase saved line */
	TTY_CTRL_ELA, /* erase line after */
	TTY_CTRL_ELB, /* erase line before */
	TTY_CTRL_EL, /* erase line */
	TTY_CTRL_GC, /* graph clear */
	TTY_CTRL_GB, /* graph bold */
	TTY_CTRL_GRB, /* graph reset bold */
	TTY_CTRL_GD, /* graph dim */
	TTY_CTRL_GRD, /* graph reset dim */
	TTY_CTRL_GI, /* graph italic */
	TTY_CTRL_GRI, /* graph reset italic */
	TTY_CTRL_GU, /* graph underline */
	TTY_CTRL_GRU, /* graph reset underline */
	TTY_CTRL_GBL, /* graph blink */
	TTY_CTRL_GRBL, /* graph reset blink */
	TTY_CTRL_GR, /* graph reverse */
	TTY_CTRL_GRR, /* graph reset reverse */
	TTY_CTRL_GH, /* graph hidden */
	TTY_CTRL_GRH, /* graph reset hidden */
	TTY_CTRL_GS, /* graph strikethrough */
	TTY_CTRL_GRS, /* graph reset strikethrough */
	TTY_CTRL_GFG, /* graph foreground color */
	TTY_CTRL_GBG, /* graph background color */
	TTY_CTRL_GFG24, /* graph foreground color 24 bits */
	TTY_CTRL_GBG24, /* graph background color 24 bits */
	TTY_CTRL_GFG256, /* graph foreground color 256 */
	TTY_CTRL_GBG256, /* graph background color 256 */
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
	int (*read)(struct tty *tty, void *data, size_t len);
	int (*write)(struct tty *tty, const void *data, size_t len);
	int (*ctrl)(struct tty *tty, enum tty_ctrl ctrl, uint32_t val);
};

int tty_create(const char *name, struct tty_op *op, struct tty **tty);
int tty_create_vga(const char *name, struct tty **tty);
int tty_input(struct tty *tty, const void *data, size_t count);
int tty_write(struct tty *tty, const void *data, size_t count);

extern struct tty *curtty;

#endif
