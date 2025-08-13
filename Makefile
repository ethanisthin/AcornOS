ASM=nasm

SRC=src
BUILD=build

$(BUILD)/main_disk.img: $(BUILD)/boot.bin
	cp $(BUILD)/boot.bin $(BUILD)/main_disk.img
	truncate -s 1440k $(BUILD)/main_disk.img