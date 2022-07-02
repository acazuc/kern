#ifndef DEV_ACPI_H
#define DEV_ACPI_H

struct rsdp;

void *acpi_find_rsdp(void);
void acpi_handle_rsdp(struct rsdp *rsdp);

#endif
