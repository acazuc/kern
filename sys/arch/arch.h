#ifndef ARCH_H
#define ARCH_H

#include "x86/x86.h"

#include <stddef.h>

struct multiboot_info;
struct vmm_ctx;

void boot(struct multiboot_info *mb_info);
struct vmm_ctx *create_vmm_ctx(void);
void *vmalloc(size_t bytes);
void vfree(void *ptr, size_t bytes);
void *vmalloc_user(size_t bytes);
void vfree_user(void *ptr, size_t bytes);

#endif
