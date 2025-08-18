#ifndef FAT16_H
#define FAT16_H

#include "../include/kernel/types.h"
#include <stdbool.h>

typedef struct __attribute__((packed)) {
    uint8_t  jmp_instr[3];    
    char     oem_name[8];            
    uint16_t bytes_per_sector;       
    uint8_t  sectors_per_cluster;    
    uint16_t reserved_sectors;       
    uint8_t  fat_count;              
    uint16_t root_entries;           
    uint16_t total_sectors_16;       
    uint8_t  media_descriptor;       
    uint16_t sectors_per_fat;        
    uint16_t sectors_per_track;      
    uint16_t heads;                  
    uint32_t hidden_sectors;         
    uint32_t total_sectors_32;       
} fat16_boot_sector_t;

typedef struct __attribute__((packed)){
    char filename[8];
    char extension[3];
    uint8_t attributes;
    uint8_t reserved;
    uint8_t  creation_time_fine;     
    uint16_t creation_time;          
    uint16_t creation_date;          
    uint16_t last_access_date;       
    uint16_t first_cluster_high;     
    uint16_t last_write_time;        
    uint16_t last_write_date;        
    uint16_t first_cluster_low;      
    uint32_t file_size;  
} fat16_dir_entry_t;

typedef struct {
    fat16_boot_sector_t boot_sector;     
    uint32_t fat_start_sector;           
    uint32_t root_dir_start_sector;      
    uint32_t data_start_sector;          
    uint32_t total_clusters;             
    uint16_t* fat_table;                 
    bool mounted;                        
} fat16_context_t;

typedef struct {
    char filename[256];              
    uint32_t file_size;              
    uint16_t first_cluster;          
    uint8_t attributes;              
    bool is_directory;               
    bool is_readonly;                
    bool is_hidden;                  
    bool is_system;                  
} fat16_file_info_t;

typedef struct {
    char current_path[256];          
    uint16_t current_dir_cluster;    
    bool is_root;                    
} fat16_dir_context_t;

#define FAT16_FREE_CLUSTER      0x0000
#define FAT16_BAD_CLUSTER       0xFFF7
#define FAT16_END_OF_CHAIN      0xFFFF
#define FAT_ATTR_READ_ONLY   0x01
#define FAT_ATTR_HIDDEN      0x02
#define FAT_ATTR_SYSTEM      0x04
#define FAT_ATTR_VOLUME_ID   0x08
#define FAT_ATTR_DIRECTORY   0x10
#define FAT_ATTR_ARCHIVE     0x20

void fat16_init(void);
bool fat16_mount(void);
void fat16_unmount(void);
bool fat16_get_file_info(const char* filename, fat16_file_info_t* info);
bool fat16_set_file_attributes(const char* filename, uint8_t attributes);
uint32_t fat16_get_file_size(const char* filename);
bool fat16_read_sector(uint32_t sector, void* buffer);
bool fat16_write_sector(uint32_t sector, const void* buffer);
bool fat16_read_sectors(uint32_t start_sector, uint32_t count, void* buffer);
bool fat16_write_sectors(uint32_t start_sector, uint32_t count, const void* buffer);
bool fat16_read_file(const char* filename, void* buffer, uint32_t size);
bool fat16_write_file(const char* filename, const void* buffer, uint32_t size);
bool fat16_create_file(const char* filename, uint8_t attributes);
bool fat16_delete_file(const char* filename);
void fat16_set_current_directory(const char* path);
const char* fat16_get_current_directory(void);
bool fat16_change_directory(const char* dirname);
int fat16_read_root_directory(fat16_dir_entry_t* entries, int max_entries);
void fat16_filename_to_83(const char* filename, char* fat_name);
void fat16_83_to_filename(const char* fat_name, char* filename);
int fat16_read_root_directory(fat16_dir_entry_t* entries, int max_entries);
bool fat16_write_directory_entry(const fat16_dir_entry_t* entry);
bool fat16_delete_directory_entry(const char* filename);
bool fat16_read_file_content(const char* filename, void* buffer, uint32_t buffer_size, uint32_t* bytes_read);
bool fat16_write_file_content(const char* filename, const void* buffer, uint32_t data_size);

void fat16_test_basic_functions(void);
void fat16_test_cluster_operations(void);
void fat16_test_filename_conversion(void);
void fat16_test_file_operations(void);
void fat16_create_boot_sector(fat16_boot_sector_t* boot_sector);
bool fat16_format_disk(void);
extern fat16_context_t fs_ctx;



#endif