#include "pit.h"
#include "arch/x86/io.h"

void pit_init()
{
	outb(0x43, 0x36);
	outb(0x40, 0x9B);
	outb(0x40, 0x2E);
}
