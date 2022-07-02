#ifndef DEV_TTY_H
#define DEV_TTY_H

#include <sys/types.h>
#include <termios.h>
#include <stdint.h>
#include <stddef.h>

#define TTY_STOPPED (1 << 0)
#define TTY_EOF     (1 << 1)

struct fs_cdev;
struct tty_op;

struct tty
{
	struct fs_cdev *cdev;
	struct termios termios;
	struct tty_op *op;
	uint8_t args[8]; /* escape codes args; XXX: resize ? */
	uint8_t args_nb;
	uint8_t wbuf[256]; /* XXX: less than 256 ? */
	uint8_t rbuf[4096]; /* XXX: resize ? */
	uint8_t line[4096]; /* for ICANON */
	uint32_t flags;
	size_t wbuf_size;
	size_t rbuf_size;
	size_t line_size;
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
	TTY_CTRL_GFGB, /* graph foreground bright */
	TTY_CTRL_GBGB, /* graph background bright */
	TTY_CTRL_S0, /* 40x25 monochrome text */
	TTY_CTRL_S1, /* 40x25 color text */
	TTY_CTRL_S2, /* 80x25 monochrome text */
	TTY_CTRL_S3, /* 80x25 color text */
	TTY_CTRL_S4, /* 320x200x4 bitmap */
	TTY_CTRL_S5, /* 320x200x1 bitmap */
	TTY_CTRL_S6, /* 640x200x1 bitmap */
	TTY_CTRL_SLW, /* enable line wrap */
	TTY_CTRL_S13, /* 320x200x24 bitmap */
	TTY_CTRL_S14, /* 640x200x4 bitmap */
	TTY_CTRL_S15, /* 640x350x1 bitmap */
	TTY_CTRL_S16, /* 640x350x4 bitmap */
	TTY_CTRL_S17, /* 640x480x2 bitmap */
	TTY_CTRL_S18, /* 640x480x4 bitmap */
	TTY_CTRL_S19, /* 320x200x8 bitmap */
	TTY_CTRL_SR, /* screen reset */
	TTY_CTRL_PCD, /* cursor disable */
	TTY_CTRL_PCE, /* cursor enable */
	TTY_CTRL_PSR, /* screen restore */
	TTY_CTRL_PSS, /* screen save */
	TTY_CTRL_PABE, /* alternative buffer enable */
	TTY_CTRL_PABD, /* alternative buffer disable */
};

struct tty_op
{
	int (*read)(struct tty *tty, void *data, size_t len);
	int (*write)(struct tty *tty, const void *data, size_t len);
	int (*ctrl)(struct tty *tty, enum tty_ctrl ctrl, uint32_t val);
};

int tty_create(const char *name, dev_t rdev, struct tty_op *op, struct tty **tty);
int tty_create_vga(const char *name, int id, struct tty **tty);
int tty_input(struct tty *tty, const void *data, size_t count);
int tty_write(struct tty *tty, const void *data, size_t count);

extern struct tty *curtty;

#endif
