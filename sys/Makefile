include $(MKDIR)/env.mk

ARCH = x86

INCLUDE_SYSDIR = include \
                 arch/$(ARCH)/include

BIN_NAME = os.bin

LDFILE = arch/$(ARCH)/linker.ld

SRC_NAME = arch/x86/boot.S \
           arch/x86/boot.c \
           arch/x86/gdt.c \
           arch/x86/gdt.s \
           arch/x86/idt.c \
           arch/x86/idt.s \
           arch/x86/isr.c \
           arch/x86/mem.c \
           arch/x86/sys.c \
           arch/x86/mp.s \
           dev/pic/pic.c \
           dev/vga/txt.c \
           dev/vga/rgb.c \
           dev/vga/vga.c \
           dev/vga/font.c \
           dev/pit/pit.c \
           dev/com/com.c \
           dev/ps2/ps2.c \
           dev/ide/ide.c \
           dev/tty/tty.c \
           dev/pci/pci.c \
           dev/acpi/acpi.c \
           dev/apic/ioapic.c \
           dev/apic/lapic.c \
           dev/rtc/rtc.c \
           lib/string.c \
           lib/printf.c \
           lib/malloc.c \
           lib/ctype.c \
           lib/stdlib.c \
           fs/vfs.c \
           fs/devfs/devfs.c \
           fs/ramfs/ramfs.c \
           kern/sched.c \
           kern/proc.c \
           kern/elf.c \
           kern/file.c \
           kern/vmm.c \

SRC_PATH = .

SRC = $(addprefix $(SRC_PATH)/, $(SRC_NAME))

OBJ_NAME1 = $(SRC_NAME:.c=.c.o)
OBJ_NAME2 = $(OBJ_NAME1:.s=.s.o)
OBJ_NAME  = $(OBJ_NAME2:.S=.S.o)

OBJ_PATH = obj

OBJ = $(addprefix $(OBJ_PATH)/, $(OBJ_NAME))

all: $(BIN_NAME)

$(LDFILE):

$(OBJ_PATH)/%.c.o: $(SRC_PATH)/%.c
	@mkdir -p $(dir $@)
	@echo "CC $<"
	@$(CC) $(CFLAGS) -fno-pie -fno-pic -c $< -o $@

$(OBJ_PATH)/%.s.o: $(SRC_PATH)/%.s
	@mkdir -p $(dir $@)
	@echo "ASM $<"
	@$(ASM) $(ASMFLAGS) $< -o $@

$(OBJ_PATH)/%.S.o: $(SRC_PATH)/%.S
	@mkdir -p $(dir $@)
	@echo "AS $<"
	@$(AS) $(ASFLAGS) $< -o $@

$(BIN_NAME): $(OBJ) $(LDFILE)
	@mkdir -p $(dir $@)
	@ld -melf_i386 -r -b binary -o obj/sh ../bin/sh/sh
	@ld -melf_i386 -r -b binary -o obj/cat ../bin/cat/cat
	@ld -melf_i386 -r -b binary -o obj/init ../bin/init/init
	@ld -melf_i386 -r -b binary -o obj/ls ../bin/ls/ls
	@ld -melf_i386 -r -b binary -o obj/libc.so ../lib/libc/libc.so
	@ld -melf_i386 -r -b binary -o obj/ld.so ../lib/ld/ld.so
	@echo "LD $@"
	@$(LD) $(LDFLAGS) -ffreestanding -T $(LDFILE) -o $@ $(OBJ) obj/sh obj/cat obj/init obj/ls obj/libc.so obj/ld.so -lgcc

clean:
	@rm -f $(OBJ)
	@rm -f $(BIN_NAME)

.PHONY: all clean
