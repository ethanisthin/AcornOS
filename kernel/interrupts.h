#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#define IDT_ENTRIES 256
#define IRQ0 32
#define IRQ1 33

/* Struct Definitions */
struct idt_entry{
    unsigned short base_low;
    unsigned short selector;
    unsigned char zero;
    unsigned char flags;
    unsigned short base_high;
} __attribute__((packed));

struct idt_descriptor {
    unsigned short limit;
    unsigned int base;
} __attribute__((packed));

/* Function Declarations */
void idt_set_gate(unsigned char num, unsigned int base, unsigned short selector, unsigned char flags);
void idt_install();
extern void idt_load();
extern void irq0_handler();
extern void irq1_handler();
void irq_handler (int irq);
void irq_handle_install(int, void(*)());
void irq_handle_uninstall(int);

#endif