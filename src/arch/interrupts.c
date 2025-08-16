#include "interrupts.h"
#include "../drivers/vga.h"
#include "pic.h"
#include "timer.h"
#include "../drivers/keyboard.h"

static idt_entry_t idt[IDT_ENTRIES];
static idt_descriptor_t idt_desc;

void idt_set_gate(uint8_t num, uint32_t handler, uint16_t selector, uint8_t flags) {
    idt[num].offset_low = handler & 0xFFFF;
    idt[num].offset_high = (handler >> 16) & 0xFFFF;
    idt[num].selector = selector;
    idt[num].zero = 0;
    idt[num].type_attr = flags;
}

void idt_load(void) {
    idt_desc.limit = (sizeof(idt_entry_t) * IDT_ENTRIES) - 1;
    idt_desc.base = (uint32_t)&idt;
    __asm__ volatile ("lidt %0" : : "m" (idt_desc));
}

void interrupt_handler(registers_t regs) {
    vga_printf_colored(VGA_COLOR_GREEN, VGA_COLOR_BLACK,
                      "Interrupt %d received! Error code: %d\n", 
                      regs.int_no, regs.err_code);
    switch(regs.int_no) {
        case 0:
            vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,
                              "FATAL: Division by zero exception!\n");
            vga_printf("System halted.\n");
            break;
        case 6:
            vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,
                              "FATAL: Invalid opcode exception!\n");
            vga_printf("System halted.\n");
            break;
        case 13:
            vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,
                              "FATAL: General protection fault!\n");
            vga_printf("System halted.\n");
            break;
        case 14:
            vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,
                              "FATAL: Page fault exception!\n");
            vga_printf("System halted.\n");
            break;
        default:
            vga_printf_colored(VGA_COLOR_CYAN, VGA_COLOR_BLACK,
                              "Unhandled interrupt: %d\n", regs.int_no);
            break;
    }

    if (regs.int_no <= 31) {  
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,
                          "CPU Exception - System Halted!\n");
        __asm__ volatile ("cli; hlt"); 
        while(1); 
    }
}

void irq_handler(registers_t regs) {
    uint8_t irq = regs.int_no - 32;
    
    switch(irq) {
        case 0:
            timer_handler();
            break;
        case 1:
            keyboard_handler();
            break;
        default:
            vga_printf_colored(VGA_COLOR_CYAN, VGA_COLOR_BLACK,
                              "Unhandled IRQ: %d\n", irq);
            break;
    }
    pic_send_eoi(irq);
}

void idt_init(void) {
    extern void isr0(void);  extern void isr1(void);  extern void isr2(void);  extern void isr3(void);
    extern void isr4(void);  extern void isr5(void);  extern void isr6(void);  extern void isr7(void);
    extern void isr8(void);  extern void isr9(void);  extern void isr10(void); extern void isr11(void);
    extern void isr12(void); extern void isr13(void); extern void isr14(void); extern void isr15(void);
    extern void isr16(void); extern void isr17(void); extern void isr18(void); extern void isr19(void);
    extern void isr20(void); extern void isr21(void); extern void isr22(void); extern void isr23(void);
    extern void isr24(void); extern void isr25(void); extern void isr26(void); extern void isr27(void);
    extern void isr28(void); extern void isr29(void); extern void isr30(void); extern void isr31(void);
    
    extern void irq0(void);  extern void irq1(void);  extern void irq2(void);  extern void irq3(void);
    extern void irq4(void);  extern void irq5(void);  extern void irq6(void);  extern void irq7(void);
    extern void irq8(void);  extern void irq9(void);  extern void irq10(void); extern void irq11(void);
    extern void irq12(void); extern void irq13(void); extern void irq14(void); extern void irq15(void);

    idt_set_gate(0,  (uint32_t)isr0,  0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(1,  (uint32_t)isr1,  0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(2,  (uint32_t)isr2,  0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(3,  (uint32_t)isr3,  0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(4,  (uint32_t)isr4,  0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(5,  (uint32_t)isr5,  0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(6,  (uint32_t)isr6,  0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(7,  (uint32_t)isr7,  0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(8,  (uint32_t)isr8,  0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(9,  (uint32_t)isr9,  0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(10, (uint32_t)isr10, 0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(11, (uint32_t)isr11, 0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(12, (uint32_t)isr12, 0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(13, (uint32_t)isr13, 0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(14, (uint32_t)isr14, 0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(15, (uint32_t)isr15, 0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(16, (uint32_t)isr16, 0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(17, (uint32_t)isr17, 0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(18, (uint32_t)isr18, 0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(19, (uint32_t)isr19, 0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(20, (uint32_t)isr20, 0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(21, (uint32_t)isr21, 0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(22, (uint32_t)isr22, 0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(23, (uint32_t)isr23, 0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(24, (uint32_t)isr24, 0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(25, (uint32_t)isr25, 0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(26, (uint32_t)isr26, 0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(27, (uint32_t)isr27, 0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(28, (uint32_t)isr28, 0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(29, (uint32_t)isr29, 0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(30, (uint32_t)isr30, 0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(31, (uint32_t)isr31, 0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(32, (uint32_t)irq0,  0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(33, (uint32_t)irq1,  0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(34, (uint32_t)irq2,  0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(35, (uint32_t)irq3,  0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(36, (uint32_t)irq4,  0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(37, (uint32_t)irq5,  0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(38, (uint32_t)irq6,  0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(39, (uint32_t)irq7,  0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(40, (uint32_t)irq8,  0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(41, (uint32_t)irq9,  0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(42, (uint32_t)irq10, 0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(43, (uint32_t)irq11, 0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(44, (uint32_t)irq12, 0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(45, (uint32_t)irq13, 0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(46, (uint32_t)irq14, 0x08, IDT_INTERRUPT_GATE);
    idt_set_gate(47, (uint32_t)irq15, 0x08, IDT_INTERRUPT_GATE);
    
    idt_load();
    vga_printf_colored(VGA_COLOR_GREEN, VGA_COLOR_BLACK, 
                      "IDT initialized with exception handlers\n");
}