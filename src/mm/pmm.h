/**
    File: pmm.h

    Description:
    - This file is the header file for the physical memory manager.
    - Contains all define statements, function declarations and structs for the memory manager

*/
#ifndef PMM_H
#define PMM_H

#include "../include/kernel/types.h"

/* Parameter definitions */
#define PMM_PAGE_SIZE 4096
#define PMM_MAX_MEMORY (256*1024*1024)
#define PMM_MAX_PAGES (PMM_MAX_MEMORY/PMM_PAGE_SIZE)
#define PMM_E820_MAP_ADDRESS 0x8000
#define PMM_E820_MAX_ENTRIES 128
#define E820_TYPE_USABLE 1
#define E820_TYPE_UNRESERVED 2
#define E820_TYPE_ACPI_RECLAIM 3
#define E820_TYPE_ACPI_NVS  4
#define E820_TYPE_BAD 5

typedef struct {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t acpi_ext;
} __attribute__((packed)) e820_entry;

/* Function Declarations */
void pmm_init(void);
void* pmm_allocate_page(void);
void pmm_free_page(void* addr);
uint32_t pmm_get_free_count(void);
uint32_t pmm_get_total_count(void);


#endif