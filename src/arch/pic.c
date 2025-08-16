#include "pic.h"
#include "../drivers/vga.h"

// Port I/O functions
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Small delay for PIC operations
static void io_wait(void) {
    outb(0x80, 0);  // Write to unused port for delay
}

// Remap the PIC to new interrupt offsets
void pic_remap(uint8_t offset1, uint8_t offset2) {
    uint8_t a1, a2;
    
    // Save current masks
    a1 = inb(PIC1_DATA);
    a2 = inb(PIC2_DATA);
    
    // Start initialization sequence (ICW1)
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    
    // ICW2: Set interrupt offsets
    outb(PIC1_DATA, offset1);
    io_wait();
    outb(PIC2_DATA, offset2);
    io_wait();
    
    // ICW3: Tell master PIC there's a slave at IRQ2
    outb(PIC1_DATA, 4);
    io_wait();
    // ICW3: Tell slave PIC its cascade identity
    outb(PIC2_DATA, 2);
    io_wait();
    
    // ICW4: Set mode
    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();
    
    // Restore saved masks
    outb(PIC1_DATA, a1);
    outb(PIC2_DATA, a2);
}

// Initialize the PIC
void pic_init(void) {
    vga_printf("Initializing PIC...\n");
    
    // Remap PIC to avoid conflicts with CPU exceptions
    pic_remap(PIC1_OFFSET, PIC2_OFFSET);
    
    // Disable all IRQs initially
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
    
    vga_printf_colored(VGA_COLOR_GREEN, VGA_COLOR_BLACK,
                      "PIC initialized - Master: IRQ %d-%d, Slave: IRQ %d-%d\n",
                      PIC1_OFFSET, PIC1_OFFSET + 7,
                      PIC2_OFFSET, PIC2_OFFSET + 7);
}

// Send End of Interrupt signal
void pic_send_eoi(uint8_t irq) {
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    outb(PIC1_COMMAND, PIC_EOI);
}

// Enable a specific IRQ
void pic_enable_irq(uint8_t irq) {
    uint16_t port;
    uint8_t value;
    
    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    
    value = inb(port) & ~(1 << irq);
    outb(port, value);
}

// Disable a specific IRQ
void pic_disable_irq(uint8_t irq) {
    uint16_t port;
    uint8_t value;
    
    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    
    value = inb(port) | (1 << irq);
    outb(port, value);
}

// Disable the PIC completely
void pic_disable(void) {
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
    vga_printf("PIC disabled\n");
}

// Get Interrupt Request Register
uint16_t pic_get_irr(void) {
    outb(PIC1_COMMAND, 0x0A);
    outb(PIC2_COMMAND, 0x0A);
    return (inb(PIC2_COMMAND) << 8) | inb(PIC1_COMMAND);
}

// Get In-Service Register
uint16_t pic_get_isr(void) {
    outb(PIC1_COMMAND, 0x0B);
    outb(PIC2_COMMAND, 0x0B);
    return (inb(PIC2_COMMAND) << 8) | inb(PIC1_COMMAND);
}