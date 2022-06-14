#include "arch/arch.h"

#include "sys/std.h"
#include "shell.h"

void kernel_main(struct multiboot_info *mb_info)
{
	shell_init();
	boot(mb_info);
	shell_input_init();

#define N (1024 * 16)
	paging_dumpinfo();
	char **s = malloc(N * sizeof(*s));
	for (size_t i = 0; i < N; ++i)
	{
		s[i] = malloc(64);
		*(s[i]) = '\0';
	}
	paging_dumpinfo();
	for (size_t i = 0; i < N; ++i)
		free(s[i]);
	paging_dumpinfo();
	free(s);
	paging_dumpinfo();

infl:
	__asm__ volatile ("hlt");
	goto infl;
}
