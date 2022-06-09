#ifndef X86_H
#define X86_H

#include <stdint.h>
#include <stddef.h>

void idt_init(void);
void reload_segments(void);
void gdt_init(void);
void paging_init(void *addr, uint32_t size);
void paging_alloc(void *addr);

#endif
