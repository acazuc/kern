global syscall

syscall:
	pushad
	mov eax, [esp + 0x24]
	mov ebx, [esp + 0x28]
	mov ecx, [esp + 0x2C]
	mov edx, [esp + 0x30]
	mov esi, [esp + 0x34]
	mov edi, [esp + 0x38]
	mov ebp, [esp + 0x3C]
	int 0x80
	mov [esp + 4 * 7], eax
	popad
	ret
