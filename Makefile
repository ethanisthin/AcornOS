ASM = nasm
CC = gcc
LD = ld
SRC = src
BUILD = build

# Kernel source files
KERNEL_SRC = $(SRC)/kernel/kernel.c
KERNEL_OBJ = $(BUILD)/kernel.o

# Compiler flags
CFLAGS = -ffreestanding -nostdlib -Wall -Wextra

.PHONY: all run clean

all: $(BUILD)/main_disk.img

# Bootloader
$(BUILD)/boot.bin: $(SRC)/boot.asm
	@mkdir -p $(BUILD)
	$(ASM) $< -f bin -o $@

# Kernel
$(KERNEL_OBJ): $(KERNEL_SRC)
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/kernel.bin: $(KERNEL_OBJ)
	$(LD) -T $(SRC)/kernel/linker.ld -o $@ $^ --oformat binary

# Disk image
$(BUILD)/main_disk.img: $(BUILD)/boot.bin $(BUILD)/kernel.bin
	@dd if=/dev/zero of=$@ bs=1M count=16
	@dd if=$(BUILD)/boot.bin of=$@ conv=notrunc
	@echo "HDD img created"

run: $(BUILD)/main_disk.img
	qemu-system-i386 -drive format=raw,file=$(BUILD)/main_disk.img -m 1M -serial stdio -vnc :0

clean:
	rm -rf $(BUILD)