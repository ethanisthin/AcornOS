ASM=nasm

SRC=src
BUILD=build

$(BUILD)/boot.bin: $(SRC)/boot.asm
	@mkdir -p $(BUILD)
	$(ASM) $< -f bin -o $@

$(BUILD)/main_disk.img: $(BUILD)/boot.bin
		@dd if=/dev/zero of=$@ bs=1M count=16
		@dd if=$(BUILD)/boot.bin of=$@ conv=notrunc
		@echo "HDD img created"

run: $(BUILD)/main_disk.img
		qemu-system-i386 -drive format=raw,file=$(BUILD)/main_disk.img -m 1M -vnc :0

clean:
	rm -rf $(BUILD)

.PHONY: run clean