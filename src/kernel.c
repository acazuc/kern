#include "arch/arch.h"
#include "dev/vga/term.h"
#include "arch/x86/io.h"

char qwerty_ascii_convert_table[256] =
{
	'\0', '\0' /* escape */, '1', '2', '3', '4', '5', '6', '7', '8', '8', '9', '0', '-', '=', '\0' /* backspace */,
	'\0' /* tab */, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', '\n', '\0' /* control */,
	'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', '\'', '`', '\0' /* shift */, '\\',
	'Z', 'X', 'C', 'V', 'B', 'N', 'M', ',', '.', '/',
	[56] = '\0' /* alt */, ' '
};

void kernel_main(void) 
{
	term_initialize();
	term_putstr("booting\n");
	boot();
infl:
	__asm__ volatile ("hlt");
	goto infl;
}
