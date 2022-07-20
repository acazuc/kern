#include "ide.h"
#include "arch/x86/asm.h"
#include <stdint.h>
#include <errno.h>

#define ATA_SR_BSY  0x80 /* busy */
#define ATA_SR_DRDY 0x40 /* drive ready */
#define ATA_SR_DF   0x20 /* drive write fault */
#define ATA_SR_DSC  0x10 /* drive seek complete */
#define ATA_SR_DRQ  0x08 /* data request ready */
#define ATA_SR_CORR 0x04 /* corrected data */
#define ATA_SR_IDX  0x02 /* index */
#define ATA_SR_ERR  0x01 /* error */

#define ATA_ER_BBK   0x80 /* bad block */
#define ATA_ER_UNC   0x40 /* uncorrectable data */
#define ATA_ER_MC    0x20 /* media changed */
#define ATA_ER_IDNF  0x10 /* ID mark not found */
#define ATA_ER_MCR   0x08 /* media change request */
#define ATA_ER_ABRT  0x04 /* command aborted */
#define ATA_ER_TK0NF 0x02 /* track 0 not found */
#define ATA_ER_AMNF  0x01 /* no address mark */

#define ATA_CMD_READ_PIO        0x20
#define ATA_CMD_READ_PIO_EXT    0x24
#define ATA_CMD_READ_DMA        0xC8
#define ATA_CMD_READ_DMA_EXT    0x25
#define ATA_CMD_WRITE_PIO       0x30
#define ATA_CMD_WRITE_PIO_EXT   0x34
#define ATA_CMD_WRITE_DMA       0xCA
#define ATA_CMD_WRITE_DMA_EXT   0x35
#define ATA_CMD_CACHE_FLUSH     0xE7
#define ATA_CMD_CACHE_FLUSH_EXT 0xEA
#define ATA_CMD_PACKET          0xA0
#define ATA_CMD_IDENTIFY_PACKET 0xA1
#define ATA_CMD_IDENTIFY        0xEC

#define ATAPI_CMD_READ  0xA8
#define ATAPI_CMD_EJECT 0x1B

#define ATA_IDENT_DEVICETYPE   0
#define ATA_IDENT_CYLINDERS    2
#define ATA_IDENT_HEADS        6
#define ATA_IDENT_SECTORS      12
#define ATA_IDENT_SERIAL       20
#define ATA_IDENT_MODEL        54
#define ATA_IDENT_CAPABILITIES 98
#define ATA_IDENT_FIELDVALID   106
#define ATA_IDENT_MAX_LBA      120
#define ATA_IDENT_COMMANDSETS  164
#define ATA_IDENT_MAX_LBA_EXT  200

#define IDE_ATA   0x00
#define IDE_ATAPI 0x01

#define ATA_MASTER 0x00
#define ATA_SLAVE  0x01

#define ATA_REG_DATA       0x00
#define ATA_REG_ERROR      0x01
#define ATA_REG_FEATURES   0x01
#define ATA_REG_SECCOUNT0  0x02
#define ATA_REG_LBA0       0x03
#define ATA_REG_LBA1       0x04
#define ATA_REG_LBA2       0x05
#define ATA_REG_HDDEVSEL   0x06
#define ATA_REG_COMMAND    0x07
#define ATA_REG_STATUS     0x07
#define ATA_REG_SECCOUNT1  0x08
#define ATA_REG_LBA3       0x09
#define ATA_REG_LBA4       0x0A
#define ATA_REG_LBA5       0x0B
#define ATA_REG_CONTROL    0x0C
#define ATA_REG_ALTSTATUS  0x0C
#define ATA_REG_DEVADDRESS 0x0D

#define ATA_PRIMARY   0x00
#define ATA_SECONDARY 0x01

#define ATA_READ  0x00
#define ATA_WRITE 0x01

#define ATA_PRIMARY_PORT_BASE   0x1F0
#define ATA_PRIMARY_PORT_CTRL   0x3F6
#define ATA_SECONDARY_PORT_BASE 0x170
#define ATA_SECONDARY_PORT_CTRL 0x376

struct channel_regs
{
	uint16_t base;  /* I/O base */
	uint16_t ctrl;  /* control base */
	uint16_t bmide; /* bus master IDE */
	uint8_t  nien;  /* nien (no interrupt) */
} g_channels[2];

struct ide_device
{
	uint8_t  reserved;     /* 0 (empty) or 1 (this drive really exists) */
	uint8_t  channel;      /* 0 (primary channel) or 1 (secondary channel) */
	uint8_t  drive;        /* 0 (master drive) or 1 (slave drive) */
	uint16_t type;         /* 0: ATA, 1:ATAPI */
	uint16_t signature;    /* drive signature */
	uint16_t capabilities; /* features */
	uint32_t command_sets; /* command sets supported */
	uint32_t size;         /* size in sectors */
	char     model[41];    /* model in string */
} g_ide_devices[4];

static void ide_write(uint8_t channel, uint8_t reg, uint8_t data)
{
	if (reg > 0x07 && reg < 0x0C)
		ide_write(channel, ATA_REG_CONTROL, 0x80 | g_channels[channel].nien);
	if (reg < 0x08)
		outb(g_channels[channel].base  + reg - 0x00, data);
	else if (reg < 0x0C)
		outb(g_channels[channel].base  + reg - 0x06, data);
	else if (reg < 0x0E)
		outb(g_channels[channel].ctrl  + reg - 0x0A, data);
	else if (reg < 0x16)
		outb(g_channels[channel].bmide + reg - 0x0E, data);
	if (reg > 0x07 && reg < 0x0C)
		ide_write(channel, ATA_REG_CONTROL, g_channels[channel].nien);
}

static uint8_t ide_read(uint8_t channel, uint8_t reg)
{
	unsigned char result;
	if (reg > 0x07 && reg < 0x0C)
		ide_write(channel, ATA_REG_CONTROL, 0x80 | g_channels[channel].nien);
	if (reg < 0x08)
		result = inb(g_channels[channel].base + reg - 0x00);
	else if (reg < 0x0C)
		result = inb(g_channels[channel].base  + reg - 0x06);
	else if (reg < 0x0E)
		result = inb(g_channels[channel].ctrl  + reg - 0x0A);
	else if (reg < 0x16)
		result = inb(g_channels[channel].bmide + reg - 0x0E);
	if (reg > 0x07 && reg < 0x0C)
		ide_write(channel, ATA_REG_CONTROL, g_channels[channel].nien);
	return result;
}

static void ide_read_buffer(uint8_t channel, uint8_t reg, uint32_t *buffer, uint32_t quads)
{
	if (reg > 0x07 && reg < 0x0C)
		ide_write(channel, ATA_REG_CONTROL, 0x80 | g_channels[channel].nien);
	if (reg < 0x08)
		insl(g_channels[channel].base  + reg - 0x00, buffer, quads);
	else if (reg < 0x0C)
		insl(g_channels[channel].base  + reg - 0x06, buffer, quads);
	else if (reg < 0x0E)
		insl(g_channels[channel].ctrl  + reg - 0x0A, buffer, quads);
	else if (reg < 0x16)
		insl(g_channels[channel].bmide + reg - 0x0E, buffer, quads);
	if (reg > 0x07 && reg < 0x0C)
		ide_write(channel, ATA_REG_CONTROL, g_channels[channel].nien);
}

static int ide_polling(uint8_t channel, uint32_t advanced_check)
{
	/* wait 400ns */
	for (int i = 0; i < 4; ++i)
		ide_read(channel, ATA_REG_ALTSTATUS);

	while (ide_read(channel, ATA_REG_STATUS) & ATA_SR_BSY)
		;

	if (advanced_check)
	{
		uint8_t state = ide_read(channel, ATA_REG_STATUS);
		if (state & ATA_SR_ERR)
			return EINVAL;
		if (state & ATA_SR_DF)
			return EIO;
		if (!(state & ATA_SR_DRQ))
			return EINVAL;
	}
	return 0;
}

void ide_init(void)
{
	uint8_t ide_buf[2048] = {0};

	uint32_t bar0 = ATA_PRIMARY_PORT_BASE;
	uint32_t bar1 = ATA_PRIMARY_PORT_CTRL;
	uint32_t bar2 = ATA_SECONDARY_PORT_BASE;
	uint32_t bar3 = ATA_SECONDARY_PORT_CTRL;
	uint32_t bar4 = 0x000;
	int count = 0;

	g_channels[ATA_PRIMARY  ].base  = bar0;
	g_channels[ATA_PRIMARY  ].ctrl  = bar1;
	g_channels[ATA_SECONDARY].base  = bar2;
	g_channels[ATA_SECONDARY].ctrl  = bar3;
	g_channels[ATA_PRIMARY  ].bmide = bar4;
	g_channels[ATA_SECONDARY].bmide = bar4;

	ide_write(ATA_PRIMARY  , ATA_REG_CONTROL, 2);
	ide_write(ATA_SECONDARY, ATA_REG_CONTROL, 2);

	for (int i = 0; i < 2; ++i)
	{
		for (int j = 0; j < 2; ++j)
		{
			unsigned char err = 0, type = IDE_ATA, status;
			g_ide_devices[count].reserved = 0;
			ide_write(i, ATA_REG_HDDEVSEL, 0xA0 | (j << 4));
			io_wait();
			ide_write(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
			io_wait();
			if (!ide_read(i, ATA_REG_STATUS))
				continue;
			while (1)
			{
				status = ide_read(i, ATA_REG_STATUS);
				if ((status & ATA_SR_ERR))
				{
					err = 1;
					break;
				}
				if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ))
					break;
			}

			if (err)
			{
				uint8_t cl = ide_read(i, ATA_REG_LBA1);
				uint8_t ch = ide_read(i, ATA_REG_LBA2);
				if (cl == 0x14 && ch == 0xEB)
					type = IDE_ATAPI;
				else if (cl == 0x69 && ch == 0x96)
					type = IDE_ATAPI;
				else
					continue;

				ide_write(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY_PACKET);
				io_wait();
			}

			ide_read_buffer(i, ATA_REG_DATA, (uint32_t*)ide_buf, 128);
			g_ide_devices[count].reserved = 1;
			g_ide_devices[count].type = type;
			g_ide_devices[count].channel = i;
			g_ide_devices[count].drive = j;
			g_ide_devices[count].signature = *((uint16_t*)(ide_buf + ATA_IDENT_DEVICETYPE));
			g_ide_devices[count].capabilities = *((uint16_t*)(ide_buf + ATA_IDENT_CAPABILITIES));
			g_ide_devices[count].command_sets = *((uint32_t*)(ide_buf + ATA_IDENT_COMMANDSETS));

			if (g_ide_devices[count].command_sets & (1 << 26)) /* 48 bit addressing */
				g_ide_devices[count].size = *((uint32_t*)(ide_buf + ATA_IDENT_MAX_LBA_EXT));
			else /* chs or 28 bit addressing */
				g_ide_devices[count].size = *((uint32_t*)(ide_buf + ATA_IDENT_MAX_LBA));

			for (int k = 0; k < 40; k += 2)
			{
				g_ide_devices[count].model[k + 0] = ide_buf[ATA_IDENT_MODEL + k + 1];
				g_ide_devices[count].model[k + 1] = ide_buf[ATA_IDENT_MODEL + k + 0];
			}
			g_ide_devices[count].model[40] = '\0';
			g_channels[i].nien = 0x2;
			ide_write(i, ATA_REG_CONTROL, g_channels[i].nien);
			count++;
		}
	}
}

static int ide_ata_access(uint8_t dir, uint8_t drive, uint32_t lba, uint8_t numsects, uint8_t *buffer)
{
	uint8_t lba_mode, cmd;
	uint8_t lba_io[6];
	uint32_t channel = g_ide_devices[drive].channel;
	uint32_t words = 256;
	uint16_t cyl;
	uint8_t head, sect;

	if (lba >= 0x10000000)
	{
		lba_mode  = 2;
		lba_io[0] = (lba & 0x000000FF) >> 0;
		lba_io[1] = (lba & 0x0000FF00) >> 8;
		lba_io[2] = (lba & 0x00FF0000) >> 16;
		lba_io[3] = (lba & 0xFF000000) >> 24;
		lba_io[4] = 0;
		lba_io[5] = 0;
		head = 0;
	}
	else if (g_ide_devices[drive].capabilities & 0x200)
	{
		lba_mode  = 1;
		lba_io[0] = (lba & 0x00000FF) >> 0;
		lba_io[1] = (lba & 0x000FF00) >> 8;
		lba_io[2] = (lba & 0x0FF0000) >> 16;
		lba_io[3] = 0;
		lba_io[4] = 0;
		lba_io[5] = 0;
		head = (lba & 0xF000000) >> 24;
	}
	else
	{
		lba_mode  = 0;
		sect = (lba % 63) + 1;
		cyl = (lba + 1  - sect) / (16 * 63);
		lba_io[0] = sect;
		lba_io[1] = (cyl >> 0) & 0xFF;
		lba_io[2] = (cyl >> 8) & 0xFF;
		lba_io[3] = 0;
		lba_io[4] = 0;
		lba_io[5] = 0;
		head = (lba + 1  - sect) % (16 * 63) / 63;
	}

	while (ide_read(channel, ATA_REG_STATUS) & ATA_SR_BSY)
		;

	if (!lba_mode)
		ide_write(channel, ATA_REG_HDDEVSEL, 0xA0 | (g_ide_devices[drive].drive << 4) | head);
	else
		ide_write(channel, ATA_REG_HDDEVSEL, 0xE0 | (g_ide_devices[drive].drive << 4) | head);
	if (lba_mode == 2)
	{
		ide_write(channel, ATA_REG_SECCOUNT1, 0);
		ide_write(channel, ATA_REG_LBA3, lba_io[3]);
		ide_write(channel, ATA_REG_LBA4, lba_io[4]);
		ide_write(channel, ATA_REG_LBA5, lba_io[5]);
	}
	ide_write(channel, ATA_REG_SECCOUNT0, numsects);
	ide_write(channel, ATA_REG_LBA0, lba_io[0]);
	ide_write(channel, ATA_REG_LBA1, lba_io[1]);
	ide_write(channel, ATA_REG_LBA2, lba_io[2]);

	if (lba_mode == 0 && dir == ATA_READ)
		cmd = ATA_CMD_READ_PIO;
	if (lba_mode == 1 && dir == ATA_READ)
		cmd = ATA_CMD_READ_PIO;
	if (lba_mode == 2 && dir == ATA_READ)
		cmd = ATA_CMD_READ_PIO_EXT;
	if (lba_mode == 0 && dir == ATA_WRITE)
		cmd = ATA_CMD_WRITE_PIO;
	if (lba_mode == 1 && dir == ATA_WRITE)
		cmd = ATA_CMD_WRITE_PIO;
	if (lba_mode == 2 && dir == ATA_WRITE)
		cmd = ATA_CMD_WRITE_PIO_EXT;
	ide_write(channel, ATA_REG_COMMAND, cmd);
	if (dir == ATA_READ)
	{
		for (int i = 0; i < numsects; i++)
		{
			int err = ide_polling(channel, 1);
			if (err)
				return err;
			insw(g_channels[channel].base, buffer, words);
			buffer += words * 2;
		}
	}
	else
	{
		for (int i = 0; i < numsects; i++)
		{
			int err = ide_polling(channel, 0);
			if (err)
				return err;
			outsw(g_channels[channel].base, buffer, words);
			buffer += words * 2;
		}
		static const char flush_cmds[] = {ATA_CMD_CACHE_FLUSH, ATA_CMD_CACHE_FLUSH, ATA_CMD_CACHE_FLUSH_EXT};
		ide_write(channel, ATA_REG_COMMAND, flush_cmds[lba_mode]);
		int err = ide_polling(channel, 0);
		if (err)
			return err;
	}
	return 0;
}

int ide_read_sectors(uint8_t drive, uint8_t numsects, uint32_t lba, void *buffer)
{
	if (drive > 3 || !g_ide_devices[drive].reserved)
		return -ENODEV;
	if (lba + numsects > g_ide_devices[drive].size && g_ide_devices[drive].type == IDE_ATA)
		return -EINVAL;
	switch (g_ide_devices[drive].type)
	{
		case IDE_ATA:
			return ide_ata_access(ATA_READ, drive, lba, numsects, buffer);
		case IDE_ATAPI:
			return -EINVAL;
		default:
			return -EINVAL;
	}
}

int ide_write_sectors(uint8_t drive, uint8_t numsects, uint32_t lba, void *buffer)
{
	if (drive > 3 || !g_ide_devices[drive].reserved)
		return -ENODEV;
	else if (lba + numsects > g_ide_devices[drive].size && g_ide_devices[drive].type == IDE_ATA)
		return -EINVAL;
	switch (g_ide_devices[drive].type)
	{
		case IDE_ATA:
			return ide_ata_access(ATA_WRITE, drive, lba, numsects, buffer);
		case IDE_ATAPI:
			return -EINVAL;
		default:
			return -EINVAL;
	}
}
