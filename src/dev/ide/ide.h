#ifndef DEV_IDE_H
#define DEV_IDE_H

#include <stdint.h>

void ide_init(void);
int ide_read_sectors(uint8_t drive, uint8_t numsects, uint32_t lba, void *buffer);
int ide_write_sectors(uint8_t drive, uint8_t numsects, uint32_t lba, void *buffer);

#endif
