ASM = nasm

AS = as

CC = gcc

LD = gcc

ASFLAGS = --32

ASMFLAGS = -f elf32

ISYSTEM = $(addprefix -isystem , $(INCLUDE_SYSDIR))

IDQUOTE = $(addprefix -iquote , $(SRC_PATH))

CFLAGS = -std=c99 \
         -m32 \
         -ffreestanding \
         -O2 \
         -Wall \
         -Wextra \
         -Wshadow \
         -fno-omit-frame-pointer \
         -mtune=generic \
         -march=i686 \
         -fno-builtin \
         -fno-stack-protector \
         -fcf-protection=none \
         -g \
         -nostdinc \
         $(ISYSTEM) \
         $(IDQUOTE) \

LDFLAGS = -nostdlib \
          -nodefaultlibs \
          -m32 \
          -nostartfiles \
