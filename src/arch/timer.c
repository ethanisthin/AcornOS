#include "timer.h"
#include "../drivers/vga.h"
#include "pic.h"

// Port I/O functions
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Timer variables
static uint32_t timer_ticks = 0;
static uint32_t timer_frequency = 0;

// Timer interrupt handler
void timer_handler(void) {
    timer_ticks++;
    
    // Print a message every second (when ticks % frequency == 0)
    if (timer_ticks % timer_frequency == 0) {
        uint32_t seconds = timer_ticks / timer_frequency;
        vga_printf_colored(VGA_COLOR_GREEN, VGA_COLOR_BLACK,
                          "Timer: %d seconds elapsed (%d ticks)\n", 
                          seconds, timer_ticks);
    }
}

// Initialize the timer
void timer_init(uint32_t frequency) {
    timer_frequency = frequency;
    timer_ticks = 0;
    
    // Calculate the divisor for the desired frequency
    uint32_t divisor = PIT_FREQUENCY / frequency;
    
    // Make sure divisor fits in 16 bits
    if (divisor > 65535) {
        divisor = 65535;
        frequency = PIT_FREQUENCY / divisor;
        timer_frequency = frequency;
    }
    
    // Send command to PIT
    uint8_t command = PIT_SELECT_CHANNEL0 | PIT_ACCESS_LOHI | PIT_MODE_RATE_GEN | PIT_BINARY_MODE;
    outb(PIT_COMMAND, command);
    
    // Send divisor (low byte first, then high byte)
    outb(PIT_CHANNEL0, divisor & 0xFF);
    outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF);
    
    vga_printf_colored(VGA_COLOR_GREEN, VGA_COLOR_BLACK,
                      "Timer initialized: %d Hz (divisor: %d)\n", 
                      frequency, divisor);
}

// Get current tick count
uint32_t timer_get_ticks(void) {
    return timer_ticks;
}

// Get elapsed seconds
uint32_t timer_get_seconds(void) {
    return timer_ticks / timer_frequency;
}

// Simple sleep function (busy wait)
void timer_sleep(uint32_t ms) {
    uint32_t target_ticks = timer_ticks + (ms * timer_frequency / 1000);
    while (timer_ticks < target_ticks) {
        __asm__ volatile ("hlt");  // Wait for next interrupt
    }
}