#include "arch/arch.h"

#include "sys/std.h"
#include "shell.h"

static void infl(void)
{
loop:
	__asm__ volatile ("hlt");
	goto loop;
}

static void userland(void)
{
	printf("userland\n");
	uint32_t ret = syscall(0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6);
	printf("syscall returned 0x%lx\n", ret);
	while (1);
}

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
	usermode(&userland);
	printf("past usermode\n");
	infl();
}
