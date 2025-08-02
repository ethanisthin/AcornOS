#include "io.h"

void outb(unsigned short port, unsigned char val){
    __asm__ volatile("outb %0, %1": : "a"(val), "Nd"(port));
}

unsigned char inb(unsigned short port){
    unsigned char ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void pic_remapper(int off1, int off2){
    unsigned char a1, a2;
    a1 = inb(PIC1_DATA);
    a2 = inb(PIC2_DATA);

    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    outb(PIC1_DATA, off1);
    outb(PIC2_DATA, off2);
    outb(PIC1_DATA, 4);
    outb(PIC2_DATA, 2);
    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);
    outb(PIC1_DATA, a1);
    outb(PIC2_DATA, a2);
}

void disable_pic(){
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}