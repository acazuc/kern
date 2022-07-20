#ifndef SYS_VMM_H
#define SYS_VMM_H

/*
 * virtual memory layout
 *
 * 0x00000000 - 0x000FFFFF (1.0 MB): unused
 * 0x00100000 - 0xBFFFFFFF (3.2 GB): userland
 * 0xC0000000 - 0xC03FFFFF (4.0 MB): kernel binary
 * 0xC0400000 - 0xC07FFFFF (4.0 MB): physical bitmap
 * 0xC0800000 - 0xC0BFFFFF (4.0 MB): kern heap page tables
 * 0xC0C00000 - 0xFFBFFFFF (1.0 GB): kern heap
 * 0xFFC00000 - 0xFFFFFFFF (4.0 MB): recursive mapping
 */

struct vmm_range
{
	uint32_t addr;
	uint32_t size;
	TAILQ_ENTRY(vmm_range) chain;
};

TAILQ_HEAD(vmm_range_head, vmm_range);

struct vmm_region
{
	uint32_t addr;
	uint32_t size;
	struct vmm_range range_0; /* always have an available item */
	struct vmm_range_head ranges; /* address-ordered */
};

struct vmm_ctx
{
	uint32_t dir[1024]; /* XXX move to arch */ /* must be at the top for page alignment */
	uint32_t dir_paddr;
	struct vmm_region region;
#if 0
	void *stack_bot;
	void *stack_top;
	size_t stack_size;
	void *data_bot;
	void *data_top;
	size_t data_size;
	void *heap_bot;
	void *heap_top;
#endif
};

#endif
