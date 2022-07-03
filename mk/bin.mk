CC = gcc

ASM = nasm -f elf32

AS = as --32

LD = gcc

CFLAGS = -std=c99 \
         -m32 \
         -ffreestanding \
         -O2 \
         -Wall \
         -Wextra \
         -fno-builtin \
         -fno-stack-protector \
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

SRC_PATH = src

SRC_FILES = $(addprefix $(SRC_PATH)/, $(SRC))

OBJ_NAME1 = $(SRC:.c=.c.o)
OBJ_NAME2 = $(OBJ_NAME1:.s=.s.o)
OBJ_NAME  = $(OBJ_NAME2:.S=.S.o)

OBJ_PATH = obj

OBJ = $(addprefix $(OBJ_PATH)/, $(OBJ_NAME))

all: $(BIN)

$(OBJ_PATH)/%.c.o: $(SRC_PATH)/%.c
	@mkdir -p $(dir $@)
	@echo "CC $<"
	@$(CC) -c $< -o $@ $(CFLAGS)

$(OBJ_PATH)/%.s.o: $(SRC_PATH)/%.s
	@mkdir -p $(dir $@)
	@echo "ASM $<"
	@$(ASM) $< -o $@

$(OBJ_PATH)/%.S.o: $(SRC_PATH)/%.S
	@mkdir -p $(dir $@)
	@echo "AS $<"
	@$(AS) $< -o $@

$(BIN): $(OBJ)
	@mkdir -p $(dir $@)
	@echo "LD $@"
	@$(LD) -o $@ $(LDFLAGS) $< -lgcc
