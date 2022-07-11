#ifndef ARCH_H
#define ARCH_H

#include "x86/x86.h"

#include <stddef.h>

struct multiboot_info;
struct vmm_ctx;
struct thread;

void boot(struct multiboot_info *mb_info);

struct vmm_ctx *vmm_ctx_create(void);
void vmm_ctx_delete(struct vmm_ctx *ctx);
struct vmm_ctx *vmm_ctx_dup(const struct vmm_ctx *ctx);
void *vmalloc(size_t bytes);
void vfree(void *ptr, size_t bytes);
void *vmalloc_user(struct vmm_ctx *ctx, void *addr, size_t bytes); /* addr is the destination vaddr (NULL for auto-find) */
void vfree_user(struct vmm_ctx *ctx, void *ptr, size_t bytes);
void *vmap(size_t paddr, size_t bytes);
void *vmap_user(struct vmm_ctx *ctx, void *ptr, size_t bytes);
void vunmap(void *ptr, size_t bytes);
void vmm_setctx(const struct vmm_ctx *ctx);

void init_trapframe_kern(struct thread *thread);
void init_trapframe_user(struct thread *thread);

#endif
