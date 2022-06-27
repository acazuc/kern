ENTRY(_start)
 
SECTIONS
{
	. = 1M;

	.multiboot.data BLOCK(4K) : ALIGN(4K)
	{
		*(.multiboot.data)
	}

	.multiboot.text BLOCK(4K) : ALIGN(4K)
	{
		*(.multiboot.text)
	}

	.multiboot.bss BLOCK(4K) : ALIGN(4K)
	{
		*(.multiboot.bss)
	}

	_boot_end = .;

	. = 4M;

	.userland.text BLOCK(4K) : ALIGN(4K)
	{
		obj/user.c.o(.text)
		obj/user.s.o(.text)
	}

	.userland.rodata BLOCK(4K) : ALIGN(4K)
	{
		obj/user.c.o(.rodata*)
		obj/user.s.o(.rodata*)
	}

	.userland.data BLOCK(4K) : ALIGN(4K)
	{
		obj/user.c.o(.data)
		obj/user.s.o(.data)
	}

	.userland.bss BLOCK(4K) : ALIGN(4K)
	{
		obj/user.c.o(COMMON)
		obj/user.s.o(COMMON)
		obj/user.c.o(.bss)
		obj/user.s.o(.bss)
	}

	. = _boot_end + 0xC0000000;

	.text BLOCK(4K) : AT(ADDR(.text) - 0xC0000000)
	{
		*(.text)
	}

	.rodata BLOCK(4K) : AT(ADDR(.rodata) - 0xC0000000)
	{
		*(.rodata*)
	}

	.data BLOCK(4K) : AT(ADDR(.data) - 0xC0000000)
	{
		*(.data)
	}

	.bss BLOCK(4K) : AT(ADDR(.bss) - 0xC0000000)
	{
		*(COMMON)
		*(.bss)
	}

	. = ALIGN(4K);
	_kernel_end = .;
}
