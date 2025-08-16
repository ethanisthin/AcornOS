#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include "../include/kernel/types.h"

typedef struct {
    uint16_t offset_low;    
    uint16_t selector;      
    uint8_t  zero;          
    uint8_t  type_attr;     
    uint16_t offset_high;   
} __attribute__((packed)) idt_entry_t;

typedef struct {
    uint16_t limit;         
    uint32_t base;          
} __attribute__((packed)) idt_descriptor_t;

typedef struct {
    uint32_t ds;                                     
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; 
    uint32_t int_no, err_code;                       
    uint32_t eip, cs, eflags, useresp, ss;          
} registers_t;

/* Defintions */
#define IDT_ENTRIES 256
#define IDT_INTERRUPT_GATE 0x8E
#define IDT_TRAP_GATE      0x8F
#define IDT_TASK_GATE      0x85

/* Function Declarations */
void idt_init(void);
void idt_set_gate(uint8_t num, uint32_t handler, uint16_t selector, uint8_t flags);
void idt_load(void);
void interrupt_handler(registers_t regs);
void irq_handler(registers_t regs);

#endif