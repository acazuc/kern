MKDIR=$(PWD)/mk

include $(MKDIR)/env.mk

BIN_NAME = sys/os.bin

ISO_NAME = os.iso

DISK_FILE = disk.qcow2

all: sys

bin:
	@make MKDIR=$(MKDIR) INCLUDE_SYSDIR=include -C bin

lib:
	@make MKDIR=$(MKDIR) INCLUDE_SYSDIR=include -C lib

sys: bin lib
	@make MKDIR=$(MKDIR) -C sys

$(BIN_NAME): sys

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

runiso: $(ISO_NAME) $(DISK_FILE)
	@qemu-system-i386 \
	-m 1024 \
	-smp cores=2,threads=2 \
	-soundhw pcspk \
	-device qemu-xhci,id=xhci \
	-device usb-ehci,id=ehci \
	-nic user,model=e1000 \
	$(ISO_NAME)

size:
	@wc `find lib bin sys -type f \( -name \*.c -o -name \*.h -o -name \*.s -o -name \*.S \)` | tail -n 1

clean:
	@rm -f $(ISO_NAME)
	@make MKDIR=$(MKDIR) -C bin clean
	@make MKDIR=$(MKDIR) -C lib clean
	@make MKDIR=$(MKDIR) -C sys clean

.PHONY: all clean run bin lib sys
