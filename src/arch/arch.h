#ifndef ARCH_H
#define ARCH_H

#include "x86/x86.h"

#include <stddef.h>

struct multiboot_info;

void boot(struct multiboot_info *mb_info);
void *vmalloc(size_t bytes);
void vfree(void *ptr, size_t bytes);

#endif
