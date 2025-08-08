#ifndef FAT12_H
#define FAT12_H

#include "../kernel/kernel.h"

/* Definitions */
#define SECTOR_SIZE 512
#define FAT12_ENTRIES 4085
#define FAT12_BAD_CLUSTER 0xFF7
#define FAT12_EOF_CLUSTER 0xFF8
#define FAT12_FREE_CLUSTER 0x000
#define FAT12_ATTR_READ_ONLY  0x01
#define FAT12_ATTR_HIDDEN     0x02
#define FAT12_ATTR_SYSTEM     0x04
#define FAT12_ATTR_VOLUME_ID  0x08
#define FAT12_ATTR_DIRECTORY  0x10
#define FAT12_ATTR_ARCHIVE    0x20

/* Struct Creation */
struct fat12_boot_sector {
    uint8_t  jump[3];           
    uint8_t  oem_name[8];       
    uint16_t bytes_per_sector;  
    uint8_t  sectors_per_cluster; 
    uint16_t reserved_sectors;  
    uint8_t  fat_count;         
    uint16_t root_entries;      
    uint16_t total_sectors;     
    uint8_t  media_descriptor;  
    uint16_t sectors_per_fat;   
    uint16_t sectors_per_track; 
    uint16_t heads;            
    uint32_t hidden_sectors;    
    uint32_t large_sectors;     
    uint8_t  drive_number;      
    uint8_t  reserved;         
    uint8_t  boot_signature;    
    uint32_t volume_id;        
    uint8_t  volume_label[11];  
    uint8_t  fs_type[8];       
} __attribute__((packed));

struct fat12_dir_entry {
    uint8_t  filename[8];       
    uint8_t  extension[3];      
    uint8_t  attributes;        
    uint8_t  reserved;          
    uint8_t  creation_time_ms;  
    uint16_t creation_time;     
    uint16_t creation_date;     
    uint16_t last_access_date;  
    uint16_t cluster_high;      
    uint16_t last_write_time;   
    uint16_t last_write_date;   
    uint16_t cluster_low;      
    uint32_t file_size;        
} __attribute__((packed));

/* Function Declarations */
int fat12_init();
int fat12_read_sector(uint32_t sector, void* buffer);
int fat12_write_sector(uint32_t sector, void* buffer);
uint16_t fat12_get_next_cluster(uint16_t cluster);
int fat12_set_next_cluster(uint16_t cluster, uint16_t next);
uint16_t fat12_find_free_cluster();
int fat12_create_file(const char* name, uint8_t attributes);
int fat12_delete_file(const char* name);
int fat12_read_file(const char* name, void* buffer, uint32_t size);
int fat12_write_file(const char* name, void* buffer, uint32_t size);
int fat12_list_directory(struct fat12_dir_entry* entries, int max_entries);
int fat12_is_initialized();

#endif