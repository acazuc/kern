global ap_trampoline
extern ap_stacks
extern ap_startup
extern ap_running
extern ap_startup_done
extern dir_page
extern reload_segments

bits 16
ap_trampoline: ;0x8000
	cli
	cld
	mov ax, 0
	mov ds, ax
	lgdt [0x8040]
	mov eax, cr0
	or eax, 1
	mov cr0, eax
	jmp 8:0x8060

align 16
gdt_table: ;0x8020
	dd 0, 0
	dd 0x0000FFFF, 0x00CF9A00
	dd 0x0000FFFF, 0x00CF9200

align 16
gdt_value: ;0x8040
	dw 23
	dd 0x8020
	dd 0, 0

align 32
bits 32
trampoline32: ;0x8060
	mov ax, 0x10
	mov ds, ax
	mov ss, ax

	mov eax, dir_page
	mov cr3, eax
	mov eax, cr0
	or eax, 0x80000000
	mov cr0, eax

.1:	pause
	cmp DWORD [ap_startup_done], 0
	jz .1

	mov eax, 1
	cpuid
	shr ebx, 24
	mov esp, [ap_stacks + ebx * 4]
	cmp esp, 0
	jz .2

	lock inc DWORD[ap_running]
	call 8:ap_startup

.2:	cli
	hlt
	jmp .2
