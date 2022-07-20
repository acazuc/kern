isr_common:
	;save ctx
	push dword [esp + 1 * 4] ;err
	push dword [esp + 5 * 4] ;ef
	push ss
	push gs
	push fs
	push es
	push ds
	push dword [esp + 10 * 4] ;cs
	push dword [esp + 10 * 4] ;eip
	push ebp
	mov ebp, esp
	add ebp, 15 * 4
	push ebp
	push edi
	push esi
	push edx
	push ecx
	push ebx
	push eax

	;if ring has been crossed, load ss & esp from int stack
	mov eax, [esp + 15 * 4]
	test eax, 0x3000
	jz .no_ring_cross_pre
	mov eax, [esp + 23 * 4] ;ss
	mov [esp + 14 * 4], eax
	mov eax, [esp + 22 * 4] ;esp
	mov [esp + 6 * 4], eax

.no_ring_cross_pre:
	;set kernel segments
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

	;call handler
	push esp
	push dword [esp + 18 * 4] ;id
	cld
	call handle_interrupt
	add esp, 2 * 4

	;restore ctx
	mov eax, [esp + 8 * 4]
	mov [esp + 19 * 4], eax ;eip
	mov eax, [esp + 9 * 4]
	mov [esp + 20 * 4], eax ;cs
	mov eax, [esp + 15 * 4]
	mov [esp + 21 * 4], eax ;ef
	test eax, 0x3000
	jz .no_ring_cross_post
	mov eax, [esp + 14 * 4]
	mov [esp + 23 * 4], eax ;ss
	mov eax, [esp + 6 * 4]
	mov [esp + 22 * 4], eax ;esp

.no_ring_cross_post:
	pop eax
	pop ebx
	pop ecx
	pop edx
	pop esi
	pop edi
	add esp, 1 * 4 ;esp
	pop ebp
	add esp, 2 * 4 ;eip, cs
	pop ds
	pop es
	pop fs
	pop gs
	push eax
	mov eax, [esp + 2 * 4]
	test eax, 0x3000
	jnz .ring_cross_end

	;set esp & ss if going to ring 0
	pop eax
	mov esp, [esp - 8 * 4]
	sub esp, 3 * 4
	iret

.ring_cross_end:
	pop eax
	add esp, 5 * 4 ;ss, ef, err, id, err
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

extern handle_interrupt

;exceptions
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
	isr_no_err i
%assign i i+1
%endrep

global g_isr_table
g_isr_table:
%assign i 0
%rep 256
	dd isr_%+i
%assign i i+1
%endrep
