all: os.bin

boot.bin: boot.asm
	nasm -f bin boot.asm -o boot.bin

kernel.o: kernel.c
	gcc -m32 -c kernel.c -o kernel.o -ffreestanding -fno-pie -nostdlib -nostartfiles -nodefaultlibs -fno-stack-protector -O0 -fno-builtin

kernel.bin: kernel.o
	ld -m elf_i386 -Ttext 0x8000 --oformat binary -o kernel.bin kernel.o -e _start --strip-all


os.bin: boot.bin kernel.bin
	cat boot.bin kernel.bin > os.bin		

run: os.bin
	qemu-system-x86_64 -drive format=raw,file=os.bin

clean:
	rm -f *.bin *.o

.PHONY: all run clean