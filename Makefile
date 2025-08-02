# all: os.bin

# boot.bin: boot.asm
# 	nasm -f bin boot.asm -o boot.bin

# idt.o: idt.asm
# 	nasm -f elf32 idt.asm -o idt.o

# kernel.o: kernel.c
# 	gcc -m32 -c kernel.c -o kernel.o -ffreestanding -fno-pie -nostdlib -nostartfiles -nodefaultlibs -fno-stack-protector -O0 -fno-builtin

# kernel.bin: kernel.o idt.o
# 	ld -m elf_i386 -Ttext 0x8000 --oformat binary -o kernel.bin kernel.o idt.o -e _start --strip-all

# os.bin: boot.bin kernel.bin
# 	cat boot.bin kernel.bin > os.bin

# run: os.bin
# 	qemu-system-x86_64 -k en-us -drive format=raw,file=os.bin -d int -no-reboot

# clean:
# 	rm -f *.bin *.o

# .PHONY: all run clean

KERNEL_SRC = kernel/kernel.c kernel/vga.c kernel/interrupts.c kernel/io.c kernel/kbm.c
KERNEL_OB = $(KERNEL_SRC:.c=.o)

all: os.bin

boot.bin: boot.asm
	nasm -f bin boot.asm -o boot.bin   

idt.o: idt.asm
	nasm -f elf32 idt.asm -o idt.o     

%.o: %.c
	gcc -m32 -c $< -o $@ -ffreestanding -fno-pie -nostdlib -nostartfiles -nodefaultlibs -fno-stack-protector -O0 -fno-builtin -Ikernel   

kernel.bin: $(KERNEL_OB) idt.o
	ld -m elf_i386 -Ttext 0x8000 --oformat binary -o kernel.bin $(KERNEL_OB) idt.o -e _start --strip-all   

os.bin: boot.bin kernel.bin
	cat boot.bin kernel.bin > os.bin   

run: os.bin
	qemu-system-x86_64 -k en-us -drive format=raw,file=os.bin -d int -no-reboot  

clean:
	rm -f *.bin *.o kernel/*.o   

.PHONY: all run clean