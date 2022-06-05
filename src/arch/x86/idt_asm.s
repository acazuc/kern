%macro isr_err 1
isr_%+%1:
	cli
	pop eax
	pushad
	cld
	push eax
	mov eax, [exception_handlers + 4 * %+%1]
	call eax
	popad
	sti
	iret
%endmacro

%macro isr_no_err 1
isr_%+%1:
	cli
	pushad
	cld
	push 0
	mov eax, [exception_handlers + 4 * %+%1]
	call eax
	pop eax
	popad
	sti
	iret
%endmacro

extern exception_handlers

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
isr_no_err 21
isr_no_err 22
isr_no_err 23
isr_no_err 24
isr_no_err 25
isr_no_err 26
isr_no_err 27
isr_no_err 28
isr_no_err 29
isr_err    30
isr_no_err 31

; IRQ master
isr_no_err  32
isr_no_err  33
isr_no_err  34
isr_no_err  35
isr_no_err  36
isr_no_err  37
isr_no_err  38
isr_no_err  39

; IRQ slave
isr_no_err  40
isr_no_err  41
isr_no_err  42
isr_no_err  43
isr_no_err  44
isr_no_err  45
isr_no_err  46
isr_no_err  47

global g_isr_table
g_isr_table:
%assign i 0
%rep    48
	dd isr_%+i ; use DQ instead if targeting 64-bit
%assign i i+1
%endrep
