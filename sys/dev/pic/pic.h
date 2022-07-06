#ifndef DEV_PIC_H
#define DEV_PIC_H

#include "arch/x86/x86.h"
#include <stdint.h>

void pic_init(uint8_t offset1, uint8_t offset2);
void pic_enable_irq(enum isa_irq_id id);
void pic_disable_irq(enum isa_irq_id id);
void pic_eoi(enum isa_irq_id id);

#endif
