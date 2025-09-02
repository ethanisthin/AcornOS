# AcornOS

A custom operating system built from scratch with a bootloader, kernel, shell, and file system support.


## Features

- **Custom Bootloader**: Two-stage bootloader written in assembly
- **32-bit Protected Mode Kernel**: Written in C with assembly components
- **VGA Text Mode Display Driver**: 80x25 character display with color support
- **Interrupt Handling System**: Custom IDT and PIC implementation
- **Keyboard Driver**: PS/2 keyboard input support
- **Shell Interface**: Command-line interface with history and tab completion
- **Text Editor**: Vim-inspired text editor with basic editing capabilities
- **File System**: FAT16 implementation with ATA driver support
- **Memory Management**: Basic string library and memory operations

## Components

### Bootloader
- Stage 1: Initial boot sector loader (512 bytes)
- Stage 2: Loads the kernel into memory and enters protected mode

### Kernel
- Main kernel entry point
- Hardware initialization (VGA, interrupts, keyboard)
- System resource management

### Shell
Commands include:
- `help` - Show available commands
- `clear` - Clear the screen
- `echo` - Display text
- `history` - Show command history
- File operations: `ls`, `cd`, `pwd`, `touch`, `rm`, `mkdir`, `rmdir`, `cp`, `mv`, `cat`, `stat`
- File system: `fstest`, `format`, `mount`
- `edit` - Launch the text editor

### Text Editor
- Vim-inspired editor with command and insert modes
- Basic file loading and saving
- Navigation and editing capabilities

### File System
- FAT16 implementation
- ATA driver for disk access
- File and directory operations

## Building

### Prerequisites

- NASM 
- GCC 
- LD 
- QEMU

On Ubuntu/Debian:
```bash
sudo apt-get install nasm gcc qemu-system-x86
```

### Build Instructions

```bash
# Clone the repository
git clone https://github.com/yourusername/AcornOS.git
cd AcornOS

# Build the OS
make

# Run in QEMU
make run
```

The build process creates:
1. `stage1.bin` - First stage bootloader
2. `stage2.bin` - Second stage bootloader
3. `kernel.bin` - The main kernel binary
4. `main_disk.img` - Complete disk image ready to run

## Running

### QEMU
```bash
make run
```

This will start QEMU with the disk image. The OS will boot automatically and present the shell interface.

### Real Hardware
To run on real hardware:
1. Write the disk image to a USB drive or floppy disk:
   ```bash
   sudo dd if=build/main_disk.img of=/dev/sdX bs=512
   ```
   (Replace `/dev/sdX` with your target device)
   
2. Boot your computer from the USB drive or floppy disk.

## Project Structure

```
AcornOS/
├── src/
│   ├── arch/         # Architecture-specific code (interrupts, PIC)
│   ├── bootloader/   # Two-stage bootloader (assembly)
│   ├── drivers/      # Device drivers (VGA, keyboard, ATA)
│   ├── filesystem/   # FAT16 file system implementation
│   ├── kernel/       # Main kernel code
│   ├── lib/          # Library functions (string operations)
│   ├── shell/        # Command-line shell
│   └── vim_editor/   # Vim-inspired text editor
├── build/            # Build output directory
└── Makefile          # Build configuration
```

## Development

### Debugging

To debug the OS, you can use QEMU's gdb stub:

```bash
qemu-system-i386 -drive format=raw,file=build/main_disk.img -s -S
```

Then connect with gdb:
```bash
gdb
(gdb) target remote localhost:1234
```

### Adding New Features

1. Implement your feature in the appropriate directory under `src/`
2. Add source files to the Makefile
3. Include necessary headers in your implementation
4. Build and test with `make && make run`

## License

This project is licensed under the MIT License

## Acknowledgments
- OSDev: https://wiki.osdev.org
