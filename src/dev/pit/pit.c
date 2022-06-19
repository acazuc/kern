#include "pit.h"
#include "arch/x86/io.h"
#include "sys/std.h"

#define DATA0 0x40
#define DATA1 0x41
#define DATA2 0x42
#define CMD   0x43

#define CMD_CHAN_0  0x00
#define CMD_CHAN_1  0x40
#define CMD_CHAN_2  0x80
#define CMD_CHAN_RB 0xC0

#define CMD_ACCESS_LATCH  0x00
#define CMD_ACCESS_LOBYTE 0x10
#define CMD_ACCESS_HIBYTE 0x20
#define CMD_ACCESS_MBYTE  0x30

#define CMD_OP_MODE0 0x00
#define CMD_OP_MODE1 0x02
#define CMD_OP_MODE2 0x04
#define CMD_OP_MODE3 0x06
#define CMD_OP_MODE4 0x08
#define CMD_OP_MODE5 0x09

#define PIC_FREQ 1193182

#define PIC_DIV  0x1000
#define PIC_INC  (1000000000ULL * PIC_DIV / PIC_FREQ)

static uint32_t g_sec;
static uint32_t g_nsec;

void pit_init()
{
	outb(CMD, CMD_CHAN_0 | CMD_ACCESS_MBYTE | CMD_OP_MODE3);
	outb(DATA0, PIC_DIV & 0xFF);
	outb(DATA0, (PIC_DIV >> 8) & 0xFF);
}

void pit_interrupt()
{
	g_nsec += PIC_INC;
	if (g_nsec >= 1000000000)
	{
		g_sec++;
		g_nsec -= 1000000000;
	}
}

void pit_gettime(struct timespec *ts)
{
	ts->tv_sec = g_sec;
	ts->tv_nsec = g_nsec;
}
