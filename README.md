<div align="center">
  <img src="https://github.com/user-attachments/assets/2e0b3ebb-0f6e-4be9-85af-dade71119abf" alt="AcornOS" height = "250 width="250">
</div>

# 🌰 AcornOS

A bare-metal x86 operating system built from scratch for learning low-level systems programming and OS development fundamentals.

## Overview

AcornOS is a project exploring operating system concepts by implementing everything from the bootloader up. The goal is to create a functional OS with user management, file operations, and a command-line interface.

## Features
- Command-line shell interface
- User authentication system
- File system implementation
- Text editor (vim-inspired)
- Directory management
- Basic file operations (create, edit, save, delete)

## Architecture
- **Type**: Monolithic kernel
- **Target**: x86 (32-bit)
- **Languages**: C and x86 Assembly
- **Memory Model**: Flat memory addressing
- **Boot**: Custom bootloader (no GRUB dependency)

## Building and Running

### Prerequisites
```bash
# Ubuntu/Debian
sudo apt install gcc-multilib nasm qemu-system-x86

# macOS (with Homebrew)
brew install i686-elf-gcc nasm qemu
```

### Build Instructions
```bash
git clone https://github.com/ethanisthin/AcornOS.git
cd AcornOS

make clean    # Clean all artifacts
make all    # Build the OS
make run  # Launch in QEMU
```

### Project Structure
```
AcornOS/
├── boot.asm          # Bootloader
├── kernel.c          # Main kernel
├── Makefile          # Build system
└── README.md
```


## Notes
- Contributions and suggestions welcome!

## Resources

- [OSDev Wiki](https://wiki.osdev.org/)

## License

MIT License - See LICENSE file for details.

