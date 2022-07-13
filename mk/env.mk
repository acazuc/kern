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
         -nostdinc \
         -isystem $(INCLUDE_SYSDIR) \
         -iquote $(SRC_PATH)

LDFLAGS = -nostdlib \
          -nodefaultlibs \
          -m32 \
          -nostartfiles
