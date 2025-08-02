#ifndef KBM_H
#define KBM_H

#define MOD_SHIFT     (1 << 0)
#define MOD_CTRL      (1 << 1)
#define MOD_ALT       (1 << 2)
#define MOD_CAPSLOCK  (1 << 3)
#define KBD_DATA_PORT    0x60
#define KBD_STATUS_PORT  0x64
#define KBD_CMD_PORT     0x64

void kbm_handler();

#endif