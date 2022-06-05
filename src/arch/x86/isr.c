#include "isr.h"

#include "arch/x86/io.h"
#include "dev/vga/term.h"

#include <stdint.h>

static void exception_handler(uint32_t err)
{
	(void)err;
	term_putstr("interrupt unk\n");
}

static void irq_handler_32(uint32_t err)
{
	(void)err;
	outb(0x20, 0x20);
}

static void irq_handler_33(uint32_t err)
{
	(void)err;
	term_putstr("IRQ 33: ");
	uint8_t c = inb(0x60);
	term_putint(c);
	term_putchar('\n');
	outb(0x20, 0x20);
}

static void irq_handler(uint32_t err)
{
	(void)err;
	term_putstr("IRQ\n");
	outb(0x20, 0x20);
}

void (*exception_handlers[256])(uint32_t) =
{
	exception_handler, /* 0 */
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler, /* 10 */
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler, /* 20 */
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler, /* 30 */
	exception_handler,
	irq_handler_32,
	irq_handler_33,
	irq_handler,
	irq_handler,
	irq_handler,
	irq_handler,
	irq_handler,
	irq_handler,
	irq_handler,/* 40 */
	irq_handler,
	irq_handler,
	irq_handler,
	irq_handler,
	irq_handler,
	irq_handler,
	irq_handler,
	exception_handler,
	exception_handler,
	exception_handler, /* 50 */
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler, /* 60 */
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler, /* 70 */
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler, /* 80 */
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler, /* 90 */
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler, /* 100 */
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler, /* 110 */
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler, /* 120 */
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler, /* 130 */
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler, /* 140 */
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler, /* 150 */
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler, /* 160 */
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler, /* 170 */
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler, /* 180 */
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler, /* 190 */
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler, /* 200 */
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler, /* 210 */
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler, /* 220 */
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler, /* 230 */
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler, /* 240 */
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler, /* 250 */
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
	exception_handler,
};
