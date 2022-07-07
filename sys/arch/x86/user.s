extern g_userland_stack
global usermode

usermode:
	mov edx, [esp + 4]
	mov ecx, [edx + 8 * 4]
	mov ebx, [edx + 6 * 4]
	mov ax, 0x23
	mov ds, ax
	mov es, ax 
	mov fs, ax 
	mov gs, ax

	push 0x23 ;ss
	push ebx  ;esp
	pushfd    ;ef
	push 0x1B ;cs
	push ecx  ;eip
	mov eax, [edx + 4 * 0]
	mov ebx, [edx + 4 * 1]
	mov ecx, [edx + 4 * 2]
	mov esi, [edx + 4 * 4]
	mov edi, [edx + 4 * 5]
	mov ebp, [edx + 4 * 7]
	mov edx, [edx + 4 * 3]
	iret
