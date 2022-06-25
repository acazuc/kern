#include <stdint.h>
#include <stddef.h>

#include "arch/arch.h"

void userland()
{
	uint32_t ret = write(1, "userland\n", 9);
loop: goto loop;
}
