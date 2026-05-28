/*
    File: vmm.c
    Description: 
    - This file contains the implementation of the virtual memory manager used by AcornOS
    - main purpose is to enable paging and return the actual physical address of a page directory

*/

#include "vmm.h"
#include "pmm.h"
#include "../drivers/vga.h"
#include "../lib/string/string.h"
#include <stdint.h>

static uint32_t page_directory[1024] __attribute__((aligned(4096)));
static uint32_t page_tables[4][1024] __attribute__((aligned(4096)));

/*
    Function: vmm_init()
    Description: 
    - This function initializes the virtual memory manager by zeroing everything, mapping IDs for 0-16 MB and
    enabling paging. 

*/
void vmm_init(){
    
    //zero everything first
    memset(page_directory, 0, sizeof(page_directory));
    memset(page_tables, 0, sizeof(page_tables));

    //mapping 0-16 mb IDs
    for (uint32_t i=0; i<VMM_IDENTITY_MAP_END; i+= VMM_PAGE_SIZE){
        uint32_t page_index = i / VMM_PAGE_SIZE;
        uint32_t table_index = page_index / VMM_PT_ENTRIES;
        uint32_t entry_index = page_index % VMM_PT_ENTRIES;
        page_tables[table_index][entry_index] = i | VMM_PAGE_PRESENT | VMM_PAGE_RW;
    }

    //enable paging, fill page directories
    for (int i=0; i<4; i++){
        page_directory[i] = (uint32_t)&page_tables[i] | VMM_PAGE_PRESENT | VMM_PAGE_RW;
    }

    __asm__ volatile ("mov %0, %%cr3" :: "r"(page_directory));
    uint32_t cr0; 
    __asm__ volatile ("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    __asm__ volatile ("mov %0, %%cr0" :: "r"(cr0));
}

/*
    Function: vmm_get_page_directory
    Description:
    - helper function to get a page directory

*/
uint32_t vmm_get_page_directory(){
    return (uint32_t)page_directory;
}
