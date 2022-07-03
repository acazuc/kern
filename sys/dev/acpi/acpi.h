#ifndef DEV_ACPI_H
#define DEV_ACPI_H

struct rsdp;

const void *acpi_find_rsdp(void);
void acpi_handle_rsdp(const struct rsdp *rsdp);

#endif
