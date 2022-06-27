extern g_userland_stack
global usermode

usermode:
	mov edx, [esp + 4]
	mov ax, 0x23
	mov ds, ax
	mov es, ax 
	mov fs, ax 
	mov gs, ax

	push 0x23
	push g_userland_stack + 4096 * 4
	pushfd
	push 0x1B
	push edx
	iret
