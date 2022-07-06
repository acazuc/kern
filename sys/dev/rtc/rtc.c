#include "rtc.h"
#include "arch/x86/x86.h"
#include "arch/x86/asm.h"

#define RTC_REGSEL 0x70
#define RTC_DATA   0x71

#define RTC_F_NMI 0x80

#define RTC_REG_B_EI 0x40

#define RTC_FREQ 1024

#define RTC_INC (1000000000ULL / RTC_FREQ) /* ~512 ns off per second */

static struct timespec g_ts;

static void rtc_interrupt(void);

static inline uint8_t rdreg(uint8_t reg)
{
	outb(RTC_REGSEL, reg);
	return inb(RTC_DATA);
}

static inline void wrreg(uint8_t reg, uint8_t v)
{
	outb(RTC_REGSEL, reg);
	outb(RTC_DATA, v);
}

void rtc_init(void)
{
	uint8_t cur = rdreg(0xB | RTC_F_NMI);
	wrreg(0xB | RTC_F_NMI, cur | RTC_REG_B_EI);
	set_isa_irq_handler(ISA_IRQ_CMOS, rtc_interrupt);
	enable_isa_irq(ISA_IRQ_CMOS);
}

static void rtc_interrupt(void)
{
	g_ts.tv_nsec += RTC_INC;
	if (g_ts.tv_nsec >= 1000000000)
	{
		g_ts.tv_sec++;
		g_ts.tv_nsec -= 1000000000;
	}
	rdreg(0xC); /* must be read to enable int again */
	isa_eoi(ISA_IRQ_CMOS);
}

void rtc_gettime(struct timespec *ts)
{
	*ts = g_ts;
}
