CC = i686-elf-gcc

ASM = nasm -f elf32

BOOT_ASM = i686-elf-as

LD = i686-elf-gcc

CFLAGS = -ffreestanding -O2 -Wall -Wextra -mno-red-zone

LDFLAGS = -ffreestanding -O2 -nostdlib

BIN_NAME = os.bin

ISO_NAME = os.iso

SRC_NAME = kernel.c \
           boot.S \
           arch/x86/boot.c \
           arch/x86/gdt.c \
           arch/x86/gdt_asm.s \
           arch/x86/idt.c \
           arch/x86/idt_asm.s \
           arch/x86/isr.c \
           dev/pic/pic.c \
           dev/vga/term.c \
           dev/pit/pit.c \

SRC_PATH = src

SRC = $(addprefix $(SRC_PATH)/, $(SRC_NAME))

OBJ_NAME1  = $(SRC_NAME:.c=.o)
OBJ_NAME2  = $(OBJ_NAME1:.s=.o)
OBJ_NAME   = $(OBJ_NAME2:.S=.o)

OBJ_PATH = obj

OBJ = $(addprefix $(OBJ_PATH)/, $(OBJ_NAME))

all: odir $(ISO_NAME)

$(OBJ_PATH)/%.o: $(SRC_PATH)/%.c
	@echo "CC $<"
	@$(CC) -c $< -o $@ -std=gnu99 $(CFLAGS) -I $(SRC_PATH)

$(OBJ_PATH)/%.o: $(SRC_PATH)/%.s
	@echo "ASM $<"
	@$(ASM) $< -o $@

$(OBJ_PATH)/%.o: $(SRC_PATH)/%.S
	@echo "ASM $<"
	@$(BOOT_ASM) $< -o $@

$(BIN_NAME): $(OBJ)
	@echo "LD $<"
	@$(LD) -T linker.ld -o $@ $(LDFLAGS) $^ -lgcc

$(ISO_NAME): $(BIN_NAME)
	@grub-mkrescue -o $@ isodir

run: $(BIN_NAME)
	@qemu-system-i386 -kernel $(BIN_NAME)  -d int,guest_errors,pcall,unimp -D qemu.log

odir:
	@mkdir -p $(OBJ_PATH)
	@mkdir -p $(OBJ_PATH)/arch
	@mkdir -p $(OBJ_PATH)/arch/x86
	@mkdir -p $(OBJ_PATH)/dev
	@mkdir -p $(OBJ_PATH)/dev/pic
	@mkdir -p $(OBJ_PATH)/dev/vga
	@mkdir -p $(OBJ_PATH)/dev/pit

clean:
	@rm -f $(OBJ)
	@rm -f $(BIN_NAME)
	@rm -f $(ISO_NAME)

.PHONY: odir all clean run
