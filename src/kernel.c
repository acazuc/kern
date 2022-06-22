#include "arch/arch.h"

#include "sys/std.h"
#include "shell.h"

void kernel_main(struct multiboot_info *mb_info)
{
	shell_init();
	boot(mb_info);
	shell_input_init();

#define N (1024 * 8)
	paging_dumpinfo();
	char **s = malloc(N * sizeof(*s), 0);
	size_t total_size = N * sizeof(*s);
	for (size_t i = 0; i < N; ++i)
	{
		size_t size = (i % (1024 * 5)) + 1;
		s[i] = malloc(size, 0);
		memset(s[i], 0, size);
		total_size += size;
	}
	paging_dumpinfo();
	printf("total size: 0x%lx bytes (0x%lx pages)\n", total_size, total_size / 4096);
	for (size_t i = 0; i < N; ++i)
		free(s[i]);
	free(s);
	printf("boot end\n");

infl:
	__asm__ volatile ("hlt");
	goto infl;
}
