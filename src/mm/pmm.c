/* 
    File: pmm.c

    Description: 
    - This file contains the implementation of the physical memory manager for AcornOS

*/

#include "pmm.h"
#include "../lib/string/string.h"

static uint32_t bitmap[PMM_MAX_PAGES/32];
static uint32_t total_pages = 0;
static uint32_t free_pages = 0;


/*
    Function: pmm_set_page()
    Description: 
    - This is a helper function that sets a single bit in the bitmap
    - Takes in a page index as parameter   
*/
static void pmm_set_page(uint32_t page_index){
    bitmap[page_index/32] |= (1 << (page_index % 32));
}

/*
    Function: pmm_clear_page()
    Description: 
    - This is a helper function that clears a single bit in the bitmap
    - Takes in a page index as parameter   
*/
static void pmm_clear_page(uint32_t page_index){
    bitmap[page_index/32] &= ~(1 << (page_index % 32));
}

/*
    Function: pmm_test_page()
    Description: 
    - This is a helper function that checks if a page is used
    - Takes in a page index as parameter   
*/
static bool pmm_test_page(uint32_t page_index){
    return (bitmap[page_index/32] >> (page_index % 32)) & 1;
}


/*
    Function: pmm_init()
    Description: 
    - This function initializes the physical memory manager
    - It reserves pages for the kernel (start and end) as well as sections such as the BIOS, stack etc.

*/
void pmm_init(void){

    // we zero off the bitmap i.e., mark all the pages as used first
    memset(bitmap, 0xFF, sizeof(bitmap));
    free_pages = 0;
    total_pages = 0;

    // reading entries from memory
    uint32_t e820_count = *(uint32_t*)PMM_E820_MAP_ADDRESS;
    e820_entry* entries = (e820_entry*)(PMM_E820_MAP_ADDRESS + sizeof(uint32_t));

    if (e820_count > PMM_E820_MAX_ENTRIES){
        e820_count = PMM_E820_MAX_ENTRIES;
    }

    // freeing usable pages from E820
    for (uint32_t i=0; i<e820_count; i++){
        if (entries[i].type != E820_TYPE_USABLE){
            continue;
        }

        uint64_t base = entries[i].base;
        uint64_t length = entries[i].length;
        
        if (base >= PMM_MAX_MEMORY){
            continue;
        }

        uint32_t start_page = (uint32_t)(base / PMM_PAGE_SIZE);
        uint32_t page_count = (uint32_t)(length / PMM_PAGE_SIZE);

        if (start_page + page_count > PMM_MAX_PAGES){
            page_count = PMM_MAX_PAGES - start_page;
        }

        for (uint32_t j=0; j<page_count; j++){
            pmm_clear_page(start_page+j);
            free_pages++;
            if (start_page + j + 1 > total_pages){
                total_pages = start_page + j + 1;
            }
        }
    }

    // reserve pages marked as reserved
    // pmm_set_page(0);
    // free_pages--;

    // reserving pages below address 0x100000 for stack, bios etc.
    for (uint32_t i=0; i<(0x100000/PMM_PAGE_SIZE); i++){
        if (!pmm_test_page(i)){
            pmm_set_page(i);
            free_pages--;
        }
    }

    // reserving pages for kernel start and end
    extern uint32_t _kernel_start;
    extern uint32_t _kernel_end;
    uint32_t kernel_start_page = (uint32_t)&_kernel_start / PMM_PAGE_SIZE;
    uint32_t kernel_end_page = ((uint32_t)&_kernel_end + PMM_PAGE_SIZE - 1) / PMM_PAGE_SIZE;

    for (uint32_t i=kernel_start_page; i<kernel_end_page; i++){
        if (!pmm_test_page(i)){
            pmm_set_page(i);
            free_pages--;
        }
    }
}


/*
    Function: pmm_alloc_page()
    Description:
    - This function allows the memory manager to allocate a page


*/
void* pmm_alloc_page(void){
    for (uint32_t i=0; i<PMM_MAX_PAGES/32; i++){
        if (bitmap[i] == 0){
            continue;
        }

        for (uint32_t j=0; j<32; j++){
            if (bitmap[i] & (1 << j)) {
                uint32_t page_index = i*32 + j;
                pmm_set_page(page_index);
                free_pages--;
                return (void*)(page_index*PMM_PAGE_SIZE);
            }
        }
    }
    return NULL;
}

/*
    Function: pmm_free_page()
    Description:
    - This function allows the memory manager to free a page

*/
void pmm_free_page(void *addr){
    uint32_t page_index = (uint32_t)addr/PMM_PAGE_SIZE;
    
    //if out of range, ignore
    if (page_index >= PMM_MAX_PAGES){
        return;
    }

    //if free already, also ignore
    if (!pmm_test_page(page_index)){
        return;
    }

    pmm_clear_page(page_index);
    free_pages++;
}


/*
    Functions: pmm_get_free_count() and pmm_get_total_count()
    Descriptions:
    - These functions allow the memory manager to get the count of free pages and the total pages

*/
uint32_t pmm_get_free_count(void){
    return free_pages;
}

uint32_t pmm_get_total_count(void){
    return total_pages;
}