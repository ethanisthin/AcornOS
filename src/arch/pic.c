#include "pic.h"
#include "../drivers/vga.h"
#include "../debug_params.h"
#include "io.h"

static void io_wait(void) {
    outb(0x80, 0);  
}

void pic_remap(uint8_t offset1, uint8_t offset2) {
    uint8_t a1, a2;
    a1 = inb(PIC1_DATA);
    a2 = inb(PIC2_DATA);
    
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    
    outb(PIC1_DATA, offset1);
    io_wait();
    outb(PIC2_DATA, offset2);
    io_wait();
    
    outb(PIC1_DATA, 4);
    io_wait();
    outb(PIC2_DATA, 2);
    io_wait();
    
    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();
    
    outb(PIC1_DATA, a1);
    outb(PIC2_DATA, a2);
}

void pic_init(void) {
    if (PIC_DEBUG){
        vga_printf("Initializing PIC...\n");
    }
    
    pic_remap(PIC1_OFFSET, PIC2_OFFSET);
    
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
    
    if (PIC_DEBUG){
        vga_printf_colored(VGA_COLOR_GREEN, VGA_COLOR_BLACK,
                      "PIC initialized - Master: IRQ %d-%d, Slave: IRQ %d-%d\n",
                      PIC1_OFFSET, PIC1_OFFSET + 7,
                      PIC2_OFFSET, PIC2_OFFSET + 7);
    }
    
}

void pic_send_eoi(uint8_t irq) {
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    outb(PIC1_COMMAND, PIC_EOI);
}

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

void pic_disable(void) {
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
    if (PIC_DEBUG){
        vga_printf("PIC disabled\n");
    }
    
}

uint16_t pic_get_irr(void) {
    outb(PIC1_COMMAND, 0x0A);
    outb(PIC2_COMMAND, 0x0A);
    return (inb(PIC2_COMMAND) << 8) | inb(PIC1_COMMAND);
}

uint16_t pic_get_isr(void) {
    outb(PIC1_COMMAND, 0x0B);
    outb(PIC2_COMMAND, 0x0B);
    return (inb(PIC2_COMMAND) << 8) | inb(PIC1_COMMAND);
}