.set MULTIBOOT_FLAG_PAGE_ALIGN,   1 << 0
.set MULTIBOOT_FLAG_MEMORY_INFO,  1 << 1
.set MULTIBOOT_FLAG_VIDEO_MODE,   1 << 2
.set MULTIBOOT_FLAG_AOUT_KLUDGE,  1 << 3
.set MULTIBOOT_FLAGS,    MULTIBOOT_FLAG_PAGE_ALIGN | MULTIBOOT_FLAG_MEMORY_INFO
.set MULTIBOOT_MAGIC,    0x1BADB002
.set MULTIBOOT_CHECKSUM, -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)
 
.section .multiboot
.align 4
.long MULTIBOOT_MAGIC
.long MULTIBOOT_FLAGS
.long MULTIBOOT_CHECKSUM
 
.section .bss
.align 16
stack_bottom:
.skip 16384
stack_top:
 
.section .text
.global _start
.type _start, @function
_start:
	mov $stack_top, %esp
	push %ebx
	call kernel_main

	cli
1:	hlt
	jmp 1b
