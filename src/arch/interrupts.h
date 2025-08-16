#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include "../include/kernel/types.h"

// IDT entry structure
typedef struct {
    uint16_t offset_low;    // Lower 16 bits of handler address
    uint16_t selector;      // Code segment selector
    uint8_t  zero;          // Always zero
    uint8_t  type_attr;     // Type and attributes
    uint16_t offset_high;   // Upper 16 bits of handler address
} __attribute__((packed)) idt_entry_t;

// IDT descriptor structure
typedef struct {
    uint16_t limit;         // Size of IDT - 1
    uint32_t base;          // Address of IDT
} __attribute__((packed)) idt_descriptor_t;

typedef struct {
    uint32_t ds;                                     // Data segment selector
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pusha
    uint32_t int_no, err_code;                       // Interrupt number and error code
    uint32_t eip, cs, eflags, useresp, ss;          // Pushed by processor automatically
} registers_t;

// Number of IDT entries
#define IDT_ENTRIES 256

// IDT type attributes
#define IDT_INTERRUPT_GATE 0x8E
#define IDT_TRAP_GATE      0x8F
#define IDT_TASK_GATE      0x85

// Function declarations
void idt_init(void);
void idt_set_gate(uint8_t num, uint32_t handler, uint16_t selector, uint8_t flags);
void idt_load(void);
void interrupt_handler(registers_t regs);
void irq_handler(registers_t regs);

#endif