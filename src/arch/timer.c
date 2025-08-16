#include "timer.h"
#include "../drivers/vga.h"
#include "pic.h"

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static uint32_t timer_ticks = 0;
static uint32_t timer_frequency = 0;

void timer_handler(void) {
    timer_ticks++;
    if (timer_ticks % timer_frequency == 0) {
        uint32_t seconds = timer_ticks / timer_frequency;
        vga_printf_colored(VGA_COLOR_GREEN, VGA_COLOR_BLACK,
                          "Timer: %d seconds elapsed (%d ticks)\n", 
                          seconds, timer_ticks);
    }
}

void timer_init(uint32_t frequency) {
    timer_frequency = frequency;
    timer_ticks = 0;
    uint32_t divisor = PIT_FREQUENCY / frequency;
    
    if (divisor > 65535) {
        divisor = 65535;
        frequency = PIT_FREQUENCY / divisor;
        timer_frequency = frequency;
    }
    
    uint8_t command = PIT_SELECT_CHANNEL0 | PIT_ACCESS_LOHI | PIT_MODE_RATE_GEN | PIT_BINARY_MODE;
    outb(PIT_COMMAND, command);
    
    outb(PIT_CHANNEL0, divisor & 0xFF);
    outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF);
    
    vga_printf_colored(VGA_COLOR_GREEN, VGA_COLOR_BLACK,
                      "Timer initialized: %d Hz (divisor: %d)\n", 
                      frequency, divisor);
}

uint32_t timer_get_ticks(void) {
    return timer_ticks;
}

uint32_t timer_get_seconds(void) {
    return timer_ticks / timer_frequency;
}

void timer_sleep(uint32_t ms) {
    uint32_t target_ticks = timer_ticks + (ms * timer_frequency / 1000);
    while (timer_ticks < target_ticks) {
        __asm__ volatile ("hlt");  
    }
}