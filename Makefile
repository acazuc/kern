MKDIR=$(PWD)/mk

include $(MKDIR)/env.mk

INCLUDE_SYSDIR = sys/include \
                 sys/arch/x86/include

BIN_NAME = os.bin

ISO_NAME = os.iso

DISK_FILE = disk.qcow2

LDFILE = sys/arch/x86/linker.ld

BIN = bin

LIB = lib

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

SRC_PATH = sys

SRC = $(addprefix $(SRC_PATH)/, $(SRC_NAME))

OBJ_NAME1 = $(SRC_NAME:.c=.c.o)
OBJ_NAME2 = $(OBJ_NAME1:.s=.s.o)
OBJ_NAME  = $(OBJ_NAME2:.S=.S.o)

OBJ_PATH = obj

OBJ = $(addprefix $(OBJ_PATH)/, $(OBJ_NAME))

all: $(BIN_NAME)

$(BIN):
	@make MKDIR=$(MKDIR) INCLUDE_SYSDIR=include -C $(BIN)

$(LIB):
	@make MKDIR=$(MKDIR) INCLUDE_SYSDIR=include -C $(LIB)

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

$(BIN_NAME): $(OBJ) $(LDFILE) lib bin
	@mkdir -p $(dir $@)
	@ld -melf_i386 -r -b binary -o obj/sh bin/sh/sh
	@ld -melf_i386 -r -b binary -o obj/cat bin/cat/cat
	@ld -melf_i386 -r -b binary -o obj/init bin/init/init
	@ld -melf_i386 -r -b binary -o obj/libc.so lib/libc/libc.so
	@ld -melf_i386 -r -b binary -o obj/ld.so lib/ld/ld.so
	@echo "LD $@"
	@$(LD) $(LDFLAGS) -ffreestanding -T $(LDFILE) -o $@ $(OBJ) obj/sh obj/cat obj/init obj/libc.so obj/ld.so -lgcc

$(ISO_NAME): $(BIN_NAME)
	@mkdir -p $(dir $<)
	@mkdir -p iso/boot
	@cp $(BIN_NAME) iso/boot
	@mkdir -p iso/boot/grub
	@cp grub.cfg iso/boot/grub
	@grub-mkrescue -o $@ iso

$(DISK_FILE):
	@echo "Creating 100M disk image"
	@qemu-img create -f qcow2 $@ 100M
	@echo "Mounting image"
	@sudo modprobe nbd max_part=8
	@sudo qemu-nbd --connect=/dev/nbd0 disk.qcow2
	@echo "Formatting fat32"
	@sudo mkfs.fat /dev/nbd0
	@sudo qemu-nbd --disconnect /dev/nbd0

run: $(BIN_NAME) $(DISK_FILE)
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

runiso: $(ISO_NAME) $(DIKS_FILE)
	@qemu-system-i386 \
	-m 1024 \
	-smp cores=2,threads=2 \
	-soundhw pcspk \
	-device qemu-xhci,id=xhci \
	-device usb-ehci,id=ehci \
	-nic user,model=e1000 \
	$(ISO_NAME)

size:
	@wc `find $(SRC_PATH) -type f \( -name \*.c -o -name \*.h -o -name \*.s -o -name \*.S \)` | tail -n 1

clean:
	@rm -f $(OBJ)
	@rm -f $(BIN_NAME)
	@rm -f $(ISO_NAME)
	@make MKDIR=$(MKDIR) -C bin clean
	@make MKDIR=$(MKDIR) -C lib clean

.PHONY: all clean run bin lib
