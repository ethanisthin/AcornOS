/*
    File: vmm.h
    Description: This file defines all the parameters and functions as required in vmm.c


*/
#ifndef VMM_H
#define VMM_H

#include "../include/kernel/types.h"

/* Parameter Definitions */
#define VMM_PAGE_SIZE       4096
#define VMM_PAGE_PRESENT    0x01
#define VMM_PAGE_RW         0x02
#define VMM_PAGE_USER       0x04
#define VMM_PD_ENTRIES      1024
#define VMM_PT_ENTRIES      1024
#define VMM_IDENTITY_MAP_END (16 * 1024 * 1024)

/* Function Declarations */
void vmm_init(void);
uint32_t vmm_get_page_directory(void);


#endif