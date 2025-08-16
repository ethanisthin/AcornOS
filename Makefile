ASM = nasm
CC = gcc
LD = ld
SRC = src
BUILD = build

# Source files
KERNEL_SRC = $(SRC)/kernel/kernel.c $(SRC)/arch/interrupts.c $(SRC)/arch/pic.c $(SRC)/arch/timer.c 
VGA_SRC = $(SRC)/drivers/vga.c
STRING_SRC = $(SRC)/lib/string/string.c
INTERRUPT_ASM = $(SRC)/arch/interrupts_handlers.asm

# Object files
KERNEL_OBJ = $(BUILD)/kernel.o $(BUILD)/interrupts.o $(BUILD)/pic.o $(BUILD)/timer.o 
VGA_OBJ = $(BUILD)/vga.o
STRING_OBJ = $(BUILD)/string.o
INTERRUPT_OBJ = $(BUILD)/interrupts_handlers.o

KERNEL_BIN = $(BUILD)/kernel.bin

CFLAGS = -ffreestanding -nostdlib -Wall -Wextra -m32 -fno-pic -fno-pie \
         -I$(SRC)/drivers -I$(SRC)/lib/string -I$(SRC)/include -I$(SRC)/kernel -I$(SRC)/arch

.PHONY: all run clean

all: $(BUILD)/main_disk.img

$(BUILD)/stage1.bin: $(SRC)/bootloader/stage1.asm
	@mkdir -p $(BUILD)
	$(ASM) $< -f bin -o $@

$(BUILD)/stage2.bin: $(SRC)/bootloader/stage2.asm
	@mkdir -p $(BUILD)
	$(ASM) $< -f bin -o $@

$(BUILD)/kernel.o: $(SRC)/kernel/kernel.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/interrupts.o: $(SRC)/arch/interrupts.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/pic.o: $(SRC)/arch/pic.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/timer.o: $(SRC)/arch/timer.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/vga.o: $(VGA_SRC)
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/string.o: $(STRING_SRC)
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/interrupts_handlers.o: $(INTERRUPT_ASM)
	@mkdir -p $(BUILD)
	$(ASM) $< -f elf32 -o $@

$(BUILD)/kernel.bin: $(KERNEL_OBJ) $(VGA_OBJ) $(STRING_OBJ) $(INTERRUPT_OBJ)
	$(LD) -m elf_i386 -T $(SRC)/kernel/linker.ld -nostdlib -o $@ $^ --oformat binary

$(BUILD)/main_disk.img: $(BUILD)/stage1.bin $(BUILD)/stage2.bin $(BUILD)/kernel.bin
	@dd if=/dev/zero of=build/main_disk.img bs=512 count=8192
	@dd if=$(BUILD)/stage1.bin of=build/main_disk.img conv=notrunc
	@dd if=$(BUILD)/stage2.bin of=build/main_disk.img conv=notrunc bs=512 seek=1 
	@dd if=$(BUILD)/kernel.bin of=build/main_disk.img conv=notrunc bs=512 seek=5
	@echo "Disk image created successfully"

run: $(BUILD)/main_disk.img
	qemu-system-i386 -drive format=raw,file=build/main_disk.img -vnc :0
clean:
	rm -rf $(BUILD)