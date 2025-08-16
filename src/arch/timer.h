#ifndef TIMER_H
#define TIMER_H

#include "../include/kernel/types.h"


#define PIT_CHANNEL0    0x40
#define PIT_CHANNEL1    0x41
#define PIT_CHANNEL2    0x42
#define PIT_COMMAND     0x43
#define PIT_FREQUENCY   1193182
#define PIT_MODE_ONESHOT    0x00
#define PIT_MODE_RATE_GEN   0x04
#define PIT_MODE_SQUARE     0x06
#define PIT_SELECT_CHANNEL0 0x00
#define PIT_ACCESS_LOHI     0x30
#define PIT_BINARY_MODE     0x00

void timer_init(uint32_t frequency);
void timer_handler(void);
uint32_t timer_get_ticks(void);
uint32_t timer_get_seconds(void);
void timer_sleep(uint32_t ms);

#endif