#include "gdt.h"
#include "idt.h"
#include "dev/pic/pic.h"
#include "dev/pit/pit.h"
#include "io.h"

void boot(void)
{
	pic_init(0x20, 0x28);
	gdt_init();
	idt_init();
	pit_init();
	__asm__ volatile ("sti");
}
