#ifndef TERMIOS_H
#define TERMIOS_H

#include <stdint.h>

#define IGNBRK (1 << 0)
#define BRKINT (1 << 1)
#define IGNPAR (1 << 2)
#define PARMRK (1 << 3)
#define INPCK  (1 << 4)
#define ISTRIP (1 << 5)
#define INLCR  (1 << 6)
#define IGNCR  (1 << 7)
#define ICRNL  (1 << 8)
#define IXON   (1 << 9)
#define IXANY  (1 << 10)
#define IXOFF  (1 << 11)

#define OPOST  (1 << 0)
#define ONLCR  (1 << 1)
#define OCRNL  (1 << 2)
#define ONOCR  (1 << 3)
#define ONLRET (1 << 4)
#define OFILL  (1 << 5)
#define OFDEL  (1 << 6)
#define NLDLY  (1 << 7)
#define CRDLY  (1 << 8)
#define TABDLY (1 << 9)
#define BSDLY  (1 << 10)
#define VTDLY  (1 << 11)
#define FFDLY  (1 << 12)

#define CSIZE  (1 << 0)
#define CSTOPB (1 << 1)
#define CREAD  (1 << 2)
#define PARENB (1 << 3)
#define PARODD (1 << 4)
#define HUPCL  (1 << 5)
#define CLOCAL (1 << 6)

#define ISIG   (1 << 0)
#define ICANON (1 << 1)
#define ECHO   (1 << 2)
#define ECHOE  (1 << 3)
#define ECHOK  (1 << 4)
#define ECHONL (1 << 5)
#define NOFLSH (1 << 6)
#define TOSTOP (1 << 7)
#define IEXTEN (1 << 8)

#define VEOF   0x0
#define VEOL   0x1
#define VERASE 0x2
#define VINTR  0x3
#define VKILL  0x4
#define VMIN   0x5
#define VQUIT  0x6
#define VSTART 0x7
#define VSTOP  0x8
#define VSUSP  0x9
#define VTIME  0xA

#define NCCS 0xB

typedef uint32_t tcflag_t;
typedef uint8_t cc_t;
typedef uint32_t speed_t;

struct termios
{
	tcflag_t c_iflag;
	tcflag_t c_oflag;
	tcflag_t c_cflag;
	tcflag_t c_lflag;
	cc_t c_cc[NCCS];
	speed_t c_ispeed;
	speed_t c_ospeed;
};

#endif
