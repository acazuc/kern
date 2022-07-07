ASM = nasm

AS = as

CC = gcc

LD = gcc

ASFLAGS = --32

ASMFLAGS = -f elf32

CFLAGS = -std=c99 \
         -m32 \
         -ffreestanding \
         -O2 \
         -Wall \
         -Wextra \
         -fno-omit-frame-pointer \
         -mtune=generic \
         -march=i686 \
         -fno-builtin \
         -fno-stack-protector \
         -fcf-protection=none \
         -g \
         -fno-pie \
         -fno-pic \
         -nostdinc \
         -isystem $(INCLUDE_DIR) \
         -iquote $(SRC_PATH)

LDFLAGS = -ffreestanding \
          -nostdlib \
          -nodefaultlibs \
          -m32

BIN_NAME = os.bin

ISO_NAME = os.iso

DISK_FILE = disk.qcow2

LDFILE = sys/arch/x86/linker.ld

BIN = bin

LIB = lib

INCLUDE_DIR = sys/include

SRC_NAME = kernel.c \
           user.c \
           user.s \
           arch/x86/boot.S \
           arch/x86/boot.c \
           arch/x86/gdt.c \
           arch/x86/gdt.s \
           arch/x86/idt.c \
           arch/x86/idt.s \
           arch/x86/isr.c \
           arch/x86/mem.c \
           arch/x86/user.s \
           arch/x86/sys.c \
           dev/pic/pic.c \
           dev/vga/vga.c \
           dev/pit/pit.c \
           dev/com/com.c \
           dev/ps2/ps2.c \
           dev/ide/ide.c \
           dev/tty/tty.c \
           dev/tty/vga.c \
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

SRC_PATH = sys

SRC = $(addprefix $(SRC_PATH)/, $(SRC_NAME))

OBJ_NAME1 = $(SRC_NAME:.c=.c.o)
OBJ_NAME2 = $(OBJ_NAME1:.s=.s.o)
OBJ_NAME  = $(OBJ_NAME2:.S=.S.o)

OBJ_PATH = obj

OBJ = $(addprefix $(OBJ_PATH)/, $(OBJ_NAME))

all: $(BIN_NAME)

$(OBJ_PATH)/%.c.o: $(SRC_PATH)/%.c
	@mkdir -p $(dir $@)
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_PATH)/%.s.o: $(SRC_PATH)/%.s
	@mkdir -p $(dir $@)
	@echo "ASM $<"
	@$(ASM) $(ASMFLAGS) $< -o $@

$(OBJ_PATH)/%.S.o: $(SRC_PATH)/%.S
	@mkdir -p $(dir $@)
	@echo "AS $<"
	@$(AS) $(ASFLAGS) $< -o $@

$(BIN):
	@make MKDIR=$(PWD)/mk -C $(BIN)

$(LIB):
	@make -I mk -C $(LIB)

$(LDFILE):

$(BIN_NAME): $(OBJ) $(LDFILE)
	@mkdir -p $(dir $@)
	@echo "LD $@"
	@$(LD) $(LDFLAGS) -T $(LDFILE) -o $@ $(OBJ) -lgcc

$(ISO_NAME): $(BIN_NAME)
	@mkdir -p $(dir $<)
	@mkdir -p isodir
	@grub-mkrescue -o $@ isodir

$(DISK_FILE):
	@echo "Creating 100M disk image"
	@qemu-img create -f qcow2 $@ 100M
	@echo "Mounting image"
	@sudo modprobe nbd max_part=8
	@sudo qemu-nbd --connect=/dev/nbd0 disk.qcow2
	@echo "Formatting fat32"
	@sudo mkfs.fat /dev/nbd0
	@sudo qemu-nbd --disconnect /dev/nbd0

run: all $(DISK_FILE)
	@qemu-system-i386 \
	-m 1024 \
	-smp cores=2,threads=2 \
	-soundhw pcspk \
	-device piix3-ide,id=ide \
	-drive id=disk,file=$(DISK_FILE),format=qcow2,if=none \
	-device ide-hd,drive=disk,bus=ide.0 \
	-device qemu-xhci,id=xhci \
	-device usb-ehci,id=ehci \
	-nic user,model=e1000 \
	-kernel $(BIN_NAME)

clean:
	@rm -f $(OBJ)
	@rm -f $(BIN_NAME)
	@rm -f $(ISO_NAME)

.PHONY: all clean run bin lib
