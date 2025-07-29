[BITS 32]

global idt_load

extern idt_desc

idt_load:
    lidt [idt_desc]
    ret