#ifndef DEV_APIC_H
#define DEV_APIC_H

#include "arch/x86/x86.h"
#include <stdint.h>

void ioapic_init(uint8_t id);
void lapic_init(void);
void ioapic_enable_irq(uint8_t id, enum isa_irq_id irqid);
void ioapic_disable_irq(uint8_t id, enum isa_irq_id irqid);
void lapic_eoi(enum isa_irq_id id);

#endif
