CC = i686-elf-gcc

ASM = nasm -f elf32

BOOT_ASM = i686-elf-as

LD = i686-elf-gcc

CFLAGS = -ffreestanding -O0 -Wall -Wextra -fno-builtin -fno-stack-protector

LDFLAGS = -ffreestanding -nostdlib -nodefaultlibs

BIN_NAME = os.bin

ISO_NAME = os.iso

SRC_NAME = kernel.c \
           shell.c \
           arch/x86/boot.S \
           arch/x86/boot.c \
           arch/x86/gdt.c \
           arch/x86/gdt.s \
           arch/x86/idt.c \
           arch/x86/idt.s \
           arch/x86/isr.c \
           arch/x86/paging.c \
           dev/pic/pic.c \
           dev/vga/vga.c \
           dev/pit/pit.c \
           dev/com/com.c \
           dev/ps2/ps2.c \
           sys/string.c \
           sys/printf.c \
           sys/malloc.c \

SRC_PATH = src

SRC = $(addprefix $(SRC_PATH)/, $(SRC_NAME))

OBJ_NAME1 = $(SRC_NAME:.c=.c.o)
OBJ_NAME2 = $(OBJ_NAME1:.s=.s.o)
OBJ_NAME  = $(OBJ_NAME2:.S=.S.o)

OBJ_PATH = obj

OBJ = $(addprefix $(OBJ_PATH)/, $(OBJ_NAME))

all: odir $(ISO_NAME)

$(OBJ_PATH)/%.c.o: $(SRC_PATH)/%.c
	@echo "CC $<"
	@$(CC) -c $< -o $@ -std=gnu99 $(CFLAGS) -I $(SRC_PATH)

$(OBJ_PATH)/%.s.o: $(SRC_PATH)/%.s
	@echo "ASM $<"
	@$(ASM) $< -o $@

$(OBJ_PATH)/%.S.o: $(SRC_PATH)/%.S
	@echo "ASM $<"
	@$(BOOT_ASM) $< -o $@

$(BIN_NAME): $(OBJ)
	@echo "LD $<"
	@$(LD) -T linker.ld -o $@ $(LDFLAGS) $^ -lgcc

$(ISO_NAME): $(BIN_NAME)
	@mkdir -p isodir
	@grub-mkrescue -o $@ isodir

run: all
	@qemu-system-i386 -m 1024 -soundhw pcspk -kernel $(BIN_NAME)

odir:
	@mkdir -p $(OBJ_PATH)
	@mkdir -p $(OBJ_PATH)/arch
	@mkdir -p $(OBJ_PATH)/arch/x86
	@mkdir -p $(OBJ_PATH)/dev
	@mkdir -p $(OBJ_PATH)/dev/pic
	@mkdir -p $(OBJ_PATH)/dev/vga
	@mkdir -p $(OBJ_PATH)/dev/pit
	@mkdir -p $(OBJ_PATH)/dev/com
	@mkdir -p $(OBJ_PATH)/dev/ps2
	@mkdir -p $(OBJ_PATH)/sys

clean:
	@rm -f $(OBJ)
	@rm -f $(BIN_NAME)
	@rm -f $(ISO_NAME)

.PHONY: odir all clean run
