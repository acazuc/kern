#include "com.h"
#include "arch/x86/io.h"
#include "arch/x86/x86.h"

#include <stdint.h>
#include <stdio.h>

#define COM1 0x3F8
#define COM2 0x2F8
#define COM3 0x3E8
#define COM4 0x2E8
#define COM5 0x5F8
#define COM6 0x4F8
#define COM7 0x5E8
#define COM8 0x4E8

static void init_port(uint16_t port)
{
	outb(port + 0x3, 0x00); /* disable DLAB */
	outb(port + 0x1, 0x00); /* disable interrupts */
	outb(port + 0x3, 0x80); /* enable DLAB */
	outb(port + 0x0, 0x01); /* set baud rate LSB (115200) */
	outb(port + 0x1, 0x00); /* set baud rate MSB */
	outb(port + 0x3, 0x03); /* disable DLAB, 8 bits, no parity, 1 stop bit */
	outb(port + 0x2, 0xC7); /* enable fifo */
	outb(port + 0x4, 0x0B); /* enable IRQ */
	outb(port + 0x4, 0x1E); /* enable loopback */
	outb(port + 0x0, 0xAE); /* send test */
	if (inb(port + 0x0) != 0xAE)
	{
		printf("port %d test failed", port);
		/* XXX: set disabled state */
		return;
	}
	outb(port + 0x4, 0x0F); /* disable loopback */
	outb(port + 0x1, 0x01); /* enable interrupts */
}

void com_putchar(char c)
{
	outb(COM1, c);
}

static void com_interrupt(void)
{
	outb(0x20, 0x20);
}

void com_init()
{
	init_port(COM1);
	set_irq_handler(4, com_interrupt);
}
