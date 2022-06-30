load_segments:
	push eax
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	pop eax
	ret

isr_common:
	;save ctx
	push gs
	push fs
	push es
	push ds
	pushad
	call load_segments

	;call handler
	mov eax, [esp + 13 * 4]
	push eax
	mov eax, [esp + 15 * 4]
	push eax
	push esp
	mov eax, [esp + 15 * 4]
	push eax
	cld
	call handle_exception
	add esp, 0x10

	;restore ctx
	popad
	pop ds
	pop es
	pop fs
	pop gs
	add esp, 0x8
	iret

%macro isr_err 1
isr_%+%1:
	cli
	push %1
	jmp isr_common
%endmacro

%macro isr_no_err 1
isr_%+%1:
	cli
	push 0
	push %1
	jmp isr_common
%endmacro

isr_128:
	cli

	;save ctx
	pushad
	push gs
	push fs
	push es
	push ds
	call load_segments

	;call handler
	push ebp
	push edi
	push esi
	push edx
	push ecx
	push ebx
	push eax
	sub esp, 4
	push esp
	push 128
	cld
	call handle_exception
	add esp, 2 * 4
	pop eax
	mov [esp + 18 * 4], eax
	add esp, 7 * 4

	;restore ctx
	pop ds
	pop es
	pop fs
	pop gs
	popad
	iret

extern handle_exception

isr_no_err 0
isr_no_err 1
isr_no_err 2
isr_no_err 3
isr_no_err 4
isr_no_err 5
isr_no_err 6
isr_no_err 7
isr_err    8
isr_no_err 9
isr_err    10
isr_err    11
isr_err    12
isr_err    13
isr_err    14
isr_no_err 15
isr_no_err 16
isr_err    17
isr_no_err 18
isr_no_err 19
isr_no_err 20
isr_err    21
isr_no_err 22
isr_no_err 23
isr_no_err 24
isr_no_err 25
isr_no_err 26
isr_no_err 27
isr_no_err 28
isr_err    29
isr_err    30
isr_no_err 31

;IRQ master
isr_no_err 32
isr_no_err 33
isr_no_err 34
isr_no_err 35
isr_no_err 36
isr_no_err 37
isr_no_err 38
isr_no_err 39

;IRQ slave
isr_no_err 40
isr_no_err 41
isr_no_err 42
isr_no_err 43
isr_no_err 44
isr_no_err 45
isr_no_err 46
isr_no_err 47

;others
%assign i 48
%rep 208
%if i != 0x80
	isr_no_err i
%endif
%assign i i+1
%endrep

global g_isr_table
g_isr_table:
%assign i 0
%rep 256
	dd isr_%+i
%assign i i+1
%endrep
