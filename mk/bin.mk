include $(MKDIR)/env.mk

SRC_PATH = src

SRC_FILES = $(addprefix $(SRC_PATH)/, $(SRC))

OBJ_NAME1 = $(SRC:.c=.c.o)
OBJ_NAME2 = $(OBJ_NAME1:.s=.s.o)
OBJ_NAME  = $(OBJ_NAME2:.S=.S.o)

OBJ_PATH = obj

OBJ = $(addprefix $(OBJ_PATH)/, $(OBJ_NAME))

LIBS_DIR = $(addprefix -L ../../lib/, $(LIB))

LIBS_LINK1 = $(addprefix -l:, $(LIB))

LIBS_LINK = $(addsuffix .so, $(LIBS_LINK1))

LIBS_INC1 = $(addsuffix /include, $(LIB))

LIBS_INC = $(addprefix -I ../../, $(LIBS_INC1))

all: $(BIN)

$(OBJ_PATH)/%.c.o: $(SRC_PATH)/%.c
	@mkdir -p $(dir $@)
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@ $(LIBS_INC)

$(OBJ_PATH)/%.s.o: $(SRC_PATH)/%.s
	@mkdir -p $(dir $@)
	@echo "ASM $<"
	@$(ASM) $(ASMFLAGS) $< -o $@

$(OBJ_PATH)/%.S.o: $(SRC_PATH)/%.S
	@mkdir -p $(dir $@)
	@echo "AS $<"
	@$(AS) $(ASFLAGS) $< -o $@

$(BIN): $(OBJ)
	@mkdir -p $(dir $@)
	@echo "LD $@"
	@$(LD) -o $@ $(LDFLAGS) $(OBJ) -lgcc $(LIBS_DIR) $(LIBS_LINK) -Wl,-dynamic-linker,/lib/ld.so
