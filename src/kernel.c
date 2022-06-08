#include "arch/arch.h"

#include "sys/std.h"
#include "shell.h"

void panic()
{
infl:
	__asm__ volatile ("cli;hlt");
	goto infl;
}

void kernel_main(struct multiboot_info *mb_info)
{
	shell_init();
	boot(mb_info);
	shell_input_init();

infl:
	__asm__ volatile ("hlt");
	goto infl;
}
