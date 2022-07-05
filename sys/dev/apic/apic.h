#ifndef DEV_APIC_H
#define DEV_APIC_H

#include <stdint.h>

void ioapic_init(uint8_t id);
void lapic_init(void);
void lapic_eoi(void);

#endif
