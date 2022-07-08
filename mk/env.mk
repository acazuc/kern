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
