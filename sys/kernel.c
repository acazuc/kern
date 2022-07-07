#include "arch/arch.h"

void kernel_main(struct multiboot_info *mb_info)
{
	boot(mb_info);
}
