global usermode
global syscall

usermode:
	mov eax, esp
	mov edx, [esp + 4]
	mov ax, 0x23
	mov ds, ax
	mov es, ax 
	mov fs, ax 
	mov gs, ax
 
	push 0x23
	push eax
	pushfd
	push 0x1B
	push edx
	iret

syscall:
	sub esp, 4
	pushad
	mov eax, [esp + 0x24]
	mov ebx, [esp + 0x28]
	mov ecx, [esp + 0x2C]
	mov edx, [esp + 0x30]
	mov esi, [esp + 0x34]
	mov edi, [esp + 0x38]
	mov ebp, [esp + 0x3C]
	int 0x80
	mov [esp + 0x20], eax
	popad
	pop eax
	ret
