#ifndef ARCH_H
#define ARCH_H

struct multiboot_info;

void boot(struct multiboot_info *mb_info);

#endif
