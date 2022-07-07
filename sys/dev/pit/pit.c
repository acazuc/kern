#include "pit.h"
#include "arch/x86/asm.h"
#include "arch/x86/x86.h"

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

#define PIT_FREQ 1193182

#define PIT_DIV  0x1000
#define PIT_INC  (1000000000ULL * PIT_DIV / PIT_FREQ) /* ~166.665 ns offset per second */

static struct timespec g_ts;

static void pit_interrupt(const struct int_ctx *ctx);

void pit_init()
{
	outb(CMD, CMD_CHAN_0 | CMD_ACCESS_MBYTE | CMD_OP_MODE3);
	outb(DATA0, PIT_DIV & 0xFF);
	outb(DATA0, (PIT_DIV >> 8) & 0xFF);
	set_isa_irq_handler(ISA_IRQ_PIT, pit_interrupt);
	enable_isa_irq(ISA_IRQ_PIT);
}

static void pit_interrupt(const struct int_ctx *ctx)
{
	(void)ctx;
	g_ts.tv_nsec += PIT_INC;
	if (g_ts.tv_nsec >= 1000000000)
	{
		g_ts.tv_sec++;
		g_ts.tv_nsec -= 1000000000;
	}
	isa_eoi(ISA_IRQ_PIT);
}

void pit_gettime(struct timespec *ts)
{
	*ts = g_ts;
}
