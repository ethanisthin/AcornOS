[BITS 32]

global idt_load
global irq0_handler
global irq1_handler

extern idt_desc
extern irq_handler ; C function: void irq_handler(int irq)

idt_load:
    lidt [idt_desc]
    ret

irq0_handler:
    pusha                    
    push dword 0                                
    call irq_handler         
    add esp, 4               
    popa                     
    iret                    

irq1_handler:
    pusha                    
    push dword 1             
    call irq_handler         
    add esp, 4               
    popa                     
    iret                     
