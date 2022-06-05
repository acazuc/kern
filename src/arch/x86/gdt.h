#ifndef GDT_H
#define GDT_H

#include <stdint.h>
#include <stddef.h>

void reload_segments(void);
void gdt_init(void);

#endif
