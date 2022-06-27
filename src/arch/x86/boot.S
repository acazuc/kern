.set MULTIBOOT_FLAG_PAGE_ALIGN,   1 << 0
.set MULTIBOOT_FLAG_MEMORY_INFO,  1 << 1
.set MULTIBOOT_FLAG_VIDEO_MODE,   1 << 2
.set MULTIBOOT_FLAG_AOUT_KLUDGE,  1 << 3
.set MULTIBOOT_FLAGS,    MULTIBOOT_FLAG_PAGE_ALIGN | MULTIBOOT_FLAG_MEMORY_INFO
.set MULTIBOOT_MAGIC,    0x1BADB002
.set MULTIBOOT_CHECKSUM, -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)
 
.section .multiboot.data, "aw"
.align 4
.long MULTIBOOT_MAGIC
.long MULTIBOOT_FLAGS
.long MULTIBOOT_CHECKSUM

.section .multiboot.bss, "aw", @nobits
dir_page:
.skip 4096
kern_tbl:
.skip 4096
user_tbl:
.skip 4096

.section .multiboot.text, "ax"
.global _start
.type _start, @function
_start:
	#setup kern tbl
	mov $3, %eax
	mov $kern_tbl, %edi
2:	mov %eax, (%edi)
	add $4096, %eax
	add $4, %edi
	cmp $(kern_tbl + 4096), %edi
	jb 2b

	#setup user tbl
	mov $0x400007, %eax
	mov $user_tbl, %edi
3:	mov %eax, (%edi)
	add $4096, %eax
	add $4, %edi
	cmp $(user_tbl + 4096), %edi
	jb 3b

	#setup dir page
	movl $(kern_tbl + 3), dir_page + 0x000 # identity
	movl $(user_tbl + 7), dir_page + 0x004 # userland
	movl $(kern_tbl + 3), dir_page + 0xC00 # high mem
	movl $(dir_page + 3), dir_page + 0xFFC # recursive

	#setup paging
	mov $dir_page, %eax
	mov %eax, %cr3
	mov %cr0, %eax
	or $0x80000000, %eax
	mov %eax, %cr0

	#let's go
	jmp _kernel_main
 
.section .bss
.align 16
stack_bottom:
.skip 16384
stack_top:
 
.section .text
.global _kernel_main
_kernel_main:
	mov $stack_top, %esp
	add $0xC0000000, %ebx # virtual address for multiboot
	push %ebx
	call kernel_main

	cli
1:	hlt
	jmp 1b
