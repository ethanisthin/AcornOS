#include "ata.h"
#include "vga.h"
#include <stdbool.h>

static bool ata_initialized = false;


static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static bool ata_wait_ready(void) {
    uint8_t status;
    int timeout = 1000000;
    
    while (timeout--) {
        status = inb(ATA_PRIMARY_IO + ATA_REG_STATUS);
        if (!(status & ATA_STATUS_BSY) && (status & ATA_STATUS_RDY)) {
            return true;
        }
    }
    
    return false;
}


static bool ata_wait_data(void) {
    uint8_t status;
    int timeout = 1000000;
    
    while (timeout--) {
        status = inb(ATA_PRIMARY_IO + ATA_REG_STATUS);
        if (!(status & ATA_STATUS_BSY) && (status & ATA_STATUS_DRQ)) {
            return true;
        }
        if (status & ATA_STATUS_ERR) {
            return false;
        }
    }
    
    return false; 
}


void ata_init(void) {
    vga_printf("Initializing ATA/IDE driver...\n");
    
    outb(ATA_PRIMARY_CTRL, 0x04);  
    outb(ATA_PRIMARY_CTRL, 0x00);  
    
    for (int i = 0; i < 1000; i++) {
        inb(ATA_PRIMARY_IO + ATA_REG_STATUS);
    }
    
    outb(ATA_PRIMARY_IO + ATA_REG_DRIVE, 0xA0);
    
    if (!ata_wait_ready()) {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,
                          "ATA drive not ready\n");
        return;
    }
    
    ata_initialized = true;
    vga_printf_colored(VGA_COLOR_GREEN, VGA_COLOR_BLACK,
                      "ATA driver initialized successfully\n");
}


bool ata_identify(void) {
    if (!ata_initialized) {
        return false;
    }
    
    outb(ATA_PRIMARY_IO + ATA_REG_DRIVE, 0xA0);
    outb(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    
    uint8_t status = inb(ATA_PRIMARY_IO + ATA_REG_STATUS);
    if (status == 0) {
        vga_printf("No ATA drive detected\n");
        return false;
    }
    
    if (!ata_wait_data()) {
        vga_printf("ATA identify failed\n");
        return false;
    }
    
    uint16_t identify_data[256];
    for (int i = 0; i < 256; i++) {
        identify_data[i] = inw(ATA_PRIMARY_IO + ATA_REG_DATA);
    }
    
    vga_printf_colored(VGA_COLOR_GREEN, VGA_COLOR_BLACK,
                      "ATA drive identified successfully\n");
    return true;
}

bool ata_read_sector(uint32_t lba, void* buffer) {
    if (!ata_initialized || !buffer) {
        return false;
    }
    
    uint16_t* buf = (uint16_t*)buffer;
    
    if (!ata_wait_ready()) {
        return false;
    }
    
    outb(ATA_PRIMARY_IO + ATA_REG_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_PRIMARY_IO + ATA_REG_SECCOUNT, 1);           
    outb(ATA_PRIMARY_IO + ATA_REG_LBA_LOW, lba & 0xFF);
    outb(ATA_PRIMARY_IO + ATA_REG_LBA_MID, (lba >> 8) & 0xFF);
    outb(ATA_PRIMARY_IO + ATA_REG_LBA_HIGH, (lba >> 16) & 0xFF);
    outb(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_READ_SECTORS);
    
    if (!ata_wait_data()) {
        return false;
    }

    for (int i = 0; i < 256; i++) {
        buf[i] = inw(ATA_PRIMARY_IO + ATA_REG_DATA);
    }
    
    return true;
}


bool ata_write_sector(uint32_t lba, const void* buffer) {
    if (!ata_initialized || !buffer) {
        return false;
    }
    
    const uint16_t* buf = (const uint16_t*)buffer;

    if (!ata_wait_ready()) {
        return false;
    }
    
    outb(ATA_PRIMARY_IO + ATA_REG_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_PRIMARY_IO + ATA_REG_SECCOUNT, 1);           
    outb(ATA_PRIMARY_IO + ATA_REG_LBA_LOW, lba & 0xFF);
    outb(ATA_PRIMARY_IO + ATA_REG_LBA_MID, (lba >> 8) & 0xFF);
    outb(ATA_PRIMARY_IO + ATA_REG_LBA_HIGH, (lba >> 16) & 0xFF);
    outb(ATA_PRIMARY_IO + ATA_REG_COMMAND, ATA_CMD_WRITE_SECTORS);
    
    if (!ata_wait_data()) {
        return false;
    }
    
    for (int i = 0; i < 256; i++) {
        outw(ATA_PRIMARY_IO + ATA_REG_DATA, buf[i]);
    }
    
    if (!ata_wait_ready()) {
        return false;
    }
    
    return true;
}