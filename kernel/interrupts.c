#include "interrupts.h"
#include "io.h"

struct idt_entry idt[IDT_ENTRIES];
struct idt_descriptor idt_desc;
void* irq_routine[16] = {0};

void idt_set_gate(unsigned char num, unsigned int base, unsigned short selector, unsigned char flags){
    idt[num].base_low = (base & 0xFFFF);
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].selector = selector;
    idt[num].zero = 0;
    idt[num].flags = flags;
}

void idt_install(){
    idt_desc.limit = (sizeof(struct idt_entry) * IDT_ENTRIES) - 1;
    idt_desc.base = (unsigned int)&idt;

    for (int i=0; i<IDT_ENTRIES; i++){
        idt_set_gate(i,0,0,0);
    }
    idt_set_gate(IRQ0, (unsigned int)irq0_handler, 0x08, 0x8E); //timer interrupt
    idt_set_gate(IRQ1, (unsigned int)irq1_handler, 0x08, 0x8E); //kbm interrupt
    idt_load();
}

void irq_handler(int irq){
    void (*handler)() = irq_routine[irq];
    if (handler){
        handler();
    }
    if (irq>=8){
        outb(PIC2_COMMAND, 0x20);
    }
    outb(PIC1_COMMAND, 0x20);
}

void irq_handle_install(int irq, void (*handler)()){
    irq_routine[irq]=handler;
}

void irq_handle_uninstall(int irq){
    irq_routine[irq]=0;
}