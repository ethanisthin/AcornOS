#include "fat16.h"
#include "../drivers/vga.h"
#include "../lib/string/string.h"
#include "../drivers/ata.h"
#include <stdbool.h>
#include "../vim_editor/editor.h"

/* Global Variables */
fat16_context_t fs_ctx;
static fat16_dir_context_t dir_ctx;


static uint32_t fat16_get_cluster_size_sectors(void) {
    return fs_ctx.boot_sector.sectors_per_cluster;
}

static uint32_t fat16_get_cluster_size_bytes(void) {
    return fs_ctx.boot_sector.sectors_per_cluster * fs_ctx.boot_sector.bytes_per_sector;
}

uint16_t fat16_get_current_dir_cluster(void) {
    return dir_ctx.current_dir_cluster;
}

static bool fat16_load_fat_table(void) {
    if (!fs_ctx.mounted) {
        return false;
    }
    
    
    uint32_t fat_size_bytes = fs_ctx.boot_sector.sectors_per_fat * fs_ctx.boot_sector.bytes_per_sector;
    uint32_t fat_entries = fat_size_bytes / 2; 
    
    vga_printf("FAT table info: %d sectors, %d bytes, %d entries\n", 
               fs_ctx.boot_sector.sectors_per_fat, fat_size_bytes, fat_entries);
    
    
    static uint16_t fat_buffer[8192]; 
    
    if (fat_entries > 8192) {
        vga_printf("FAT table too large for buffer (need %d, have 8192)\n", fat_entries);
        return false;
    }
    
    
    uint8_t sector_buffer[512];
    uint16_t* fat_ptr = fat_buffer;
    
    vga_printf("Loading FAT table from sector %d...\n", fs_ctx.fat_start_sector);
    
    for (uint32_t sector = 0; sector < fs_ctx.boot_sector.sectors_per_fat; sector++) {
        if (!fat16_read_sector(fs_ctx.fat_start_sector + sector, sector_buffer)) {
            vga_printf("Failed to read FAT sector %d\n", sector);
            return false;
        }
        
        
        memcpy(fat_ptr, sector_buffer, 512);
        fat_ptr += 256; 
    }
    
    
    fs_ctx.fat_table = fat_buffer;
    
    vga_printf("FAT table loaded successfully (%d entries)\n", fat_entries);
    return true;
}

static bool fat16_save_fat_table(void) {
    if (!fs_ctx.mounted || !fs_ctx.fat_table) {
        return false;
    }
    
    uint32_t fat_size_bytes = fs_ctx.boot_sector.sectors_per_fat * fs_ctx.boot_sector.bytes_per_sector;
    
    for (int fat_copy = 0; fat_copy < fs_ctx.boot_sector.fat_count; fat_copy++) {
        uint32_t fat_start = fs_ctx.fat_start_sector + (fat_copy * fs_ctx.boot_sector.sectors_per_fat);
        if (!fat16_write_sectors(fat_start, fs_ctx.boot_sector.sectors_per_fat, fs_ctx.fat_table)) {
            vga_printf("Failed to write FAT copy %d\n", fat_copy);
            return false;
        }
    }
    
    return true;
}


static bool fat16_read_cluster_chain(uint16_t first_cluster, void* buffer, uint32_t buffer_size, uint32_t* bytes_read) {
    if (!fs_ctx.mounted || !fs_ctx.fat_table || !buffer || !bytes_read) {
        return false;
    }
    
    *bytes_read = 0;
    uint8_t* buf_ptr = (uint8_t*)buffer;
    uint16_t current_cluster = first_cluster;
    uint32_t cluster_size = fat16_get_cluster_size_bytes();
    uint8_t cluster_buffer[512]; 
    
    
    while (current_cluster >= 2 && current_cluster < 0xFFF8 && *bytes_read < buffer_size) {
        
        uint32_t cluster_sector = fat16_cluster_to_sector(current_cluster);
        
        if (!fat16_read_sector(cluster_sector, cluster_buffer)) {
            vga_printf("Failed to read cluster %d\n", current_cluster);
            return false;
        }
        
        
        uint32_t bytes_to_copy = cluster_size;
        if (*bytes_read + bytes_to_copy > buffer_size) {
            bytes_to_copy = buffer_size - *bytes_read;
        }
        
        memcpy(buf_ptr + *bytes_read, cluster_buffer, bytes_to_copy);
        *bytes_read += bytes_to_copy;
        
        
        if (current_cluster >= fs_ctx.total_clusters + 2) {
            vga_printf("Invalid cluster number: %d\n", current_cluster);
            break;
        }
        
        current_cluster = fs_ctx.fat_table[current_cluster];
        
        
        if (current_cluster >= 0xFFF8) {
            break; 
        }
    }
    
    return true;
}


bool fat16_update_root_directory_entry(const char* filename, uint32_t new_size, uint16_t new_first_cluster) {
    if (!fs_ctx.mounted || !filename) {
        return false;
    }
    
    uint32_t root_dir_sectors = (fs_ctx.boot_sector.root_entries * 32) / fs_ctx.boot_sector.bytes_per_sector;
    uint32_t entries_per_sector = fs_ctx.boot_sector.bytes_per_sector / 32;
    
    uint8_t sector_buffer[512];
    
    for (uint32_t sector = 0; sector < root_dir_sectors; sector++) {
        if (!fat16_read_sector(fs_ctx.root_dir_start_sector + sector, sector_buffer)) {
            return false;
        }
        
        fat16_dir_entry_t* sector_entries = (fat16_dir_entry_t*)sector_buffer;
        
        for (uint32_t i = 0; i < entries_per_sector; i++) {
            fat16_dir_entry_t* entry = &sector_entries[i];
            
            if (entry->filename[0] == 0x00 || (unsigned char)entry->filename[0] == 0xE5) {
                continue;
            }
            
            char entry_filename[13];
            fat16_83_to_filename(entry->filename, entry_filename);
            
            if (strcmp(entry_filename, filename) == 0) {
                entry->file_size = new_size;
                entry->first_cluster_low = new_first_cluster;
                
                if (!fat16_write_sector(fs_ctx.root_dir_start_sector + sector, sector_buffer)) {
                    return false;
                }
                return true;
            }
        }
    }
    
    return false;
}

static void fat16_init_directory_context(void) {
    strcpy(dir_ctx.current_path, "/");
    dir_ctx.current_dir_cluster = 0;  
    dir_ctx.is_root = true;
}

void fat16_init(void) {
    vga_printf("Initializing FAT-16 filesystem...\n");
    memset(&fs_ctx, 0, sizeof(fat16_context_t));
    fs_ctx.mounted = false;
    fs_ctx.fat_table = NULL;
    
    fat16_init_directory_context();
    vga_printf_colored(VGA_COLOR_GREEN, VGA_COLOR_BLACK,
                      "FAT-16 filesystem initialized\n");
}

bool fat16_mount(void) {
    vga_printf("Mounting FAT-16 filesystem...\n");
    
    
    fat16_boot_sector_t boot_sector;
    if (!fat16_read_sector(100, &boot_sector)) {
        vga_printf("Failed to read boot sector\n");
        return false;
    }
    
    
    if (boot_sector.bytes_per_sector != 512) {
        vga_printf("Invalid bytes per sector: %d\n", boot_sector.bytes_per_sector);
        return false;
    }
    
    if (boot_sector.fat_count != 2) {
        vga_printf("Invalid FAT count: %d\n", boot_sector.fat_count);
        return false;
    }
    
    
    if (memcmp(boot_sector.oem_name, "ACORNOS ", 8) != 0) {
        vga_printf("Invalid OEM name (not AcornOS filesystem)\n");
        return false;
    }
    
    
    memcpy(&fs_ctx.boot_sector, &boot_sector, sizeof(fat16_boot_sector_t));
    
    
    fs_ctx.fat_start_sector = 100 + boot_sector.reserved_sectors;
    fs_ctx.root_dir_start_sector = fs_ctx.fat_start_sector + (boot_sector.fat_count * boot_sector.sectors_per_fat);
    fs_ctx.data_start_sector = fs_ctx.root_dir_start_sector + ((boot_sector.root_entries * 32) / boot_sector.bytes_per_sector);
    fs_ctx.total_clusters = (boot_sector.total_sectors_16 - fs_ctx.data_start_sector) / boot_sector.sectors_per_cluster;
    
    
    fs_ctx.mounted = true;
    
    vga_printf_colored(VGA_COLOR_GREEN, VGA_COLOR_BLACK,
                      "FAT-16 filesystem mounted successfully!\n");
    vga_printf("FAT starts at sector: %d\n", fs_ctx.fat_start_sector);
    vga_printf("Root directory at sector: %d\n", fs_ctx.root_dir_start_sector);
    vga_printf("Data area at sector: %d\n", fs_ctx.data_start_sector);
    vga_printf("Total clusters: %d\n", fs_ctx.total_clusters);

    if (!fat16_load_fat_table()) {
        vga_printf("Failed to load FAT table\n");
        fs_ctx.mounted = false;
        return false;
    }
    
    return true;
}

void fat16_unmount(void) {
    if (!fs_ctx.mounted) {
        return;
    }
    vga_printf("Unmounting FAT-16 filesystem...\n");
    fs_ctx.mounted = false;
    fs_ctx.fat_table = NULL;
    
    vga_printf_colored(VGA_COLOR_GREEN, VGA_COLOR_BLACK,
                      "Filesystem unmounted\n");
}

static uint16_t fat16_find_free_cluster(void){
    if (!fs_ctx.mounted || !fs_ctx.fat_table) {
        return 0;
    }

    for (uint32_t cluster = 2; cluster < fs_ctx.total_clusters + 2; cluster++) {
        if (fs_ctx.fat_table[cluster] == FAT16_FREE_CLUSTER) {
            return cluster;
        }
    }
    
    return 0; 
}

static bool fat16_allocate_cluster(uint16_t cluster) {
    if (!fs_ctx.mounted || !fs_ctx.fat_table || cluster < 2) {
        return false;
    }
    
    if (cluster >= fs_ctx.total_clusters + 2) {
        return false;
    }
    
    fs_ctx.fat_table[cluster] = FAT16_END_OF_CHAIN;
    return true;
}

static uint16_t fat16_get_next_cluster(uint16_t cluster) {
    if (!fs_ctx.mounted || !fs_ctx.fat_table || cluster < 2) {
        return 0;
    }
    
    if (cluster >= fs_ctx.total_clusters + 2) {
        return 0;
    }
    
    return fs_ctx.fat_table[cluster];
}

static bool fat16_link_clusters(uint16_t cluster, uint16_t next_cluster) {
    if (!fs_ctx.mounted || !fs_ctx.fat_table || cluster < 2) {
        return false;
    }
    
    if (cluster >= fs_ctx.total_clusters + 2) {
        return false;
    }
    
    fs_ctx.fat_table[cluster] = next_cluster;
    return true;
}


static uint16_t fat16_allocate_file_clusters(uint32_t file_size) {
    if (!fs_ctx.mounted || file_size == 0) {
        return 0;
    }
    
    uint32_t cluster_size = fat16_get_cluster_size_bytes();
    uint32_t clusters_needed = (file_size + cluster_size - 1) / cluster_size;
    
    uint16_t first_cluster = fat16_find_free_cluster();
    if (first_cluster == 0) {
        return 0; 
    }
    
    uint16_t current_cluster = first_cluster;
    
    for (uint32_t i = 0; i < clusters_needed; i++) {
        if (i == clusters_needed - 1) {
            fat16_allocate_cluster(current_cluster);
        } else {
            uint16_t next_cluster = fat16_find_free_cluster();
            if (next_cluster == 0) {
                
                return 0;
            }
            
            fat16_link_clusters(current_cluster, next_cluster);
            current_cluster = next_cluster;
        }
    }
    
    return first_cluster;
}

void fat16_filename_to_83(const char* filename, char* fat_name) {
    memset(fat_name, ' ', 11);    
    const char* dot = NULL;
    int name_len = 0;

    for (int i = 0; filename[i] && i < 255; i++) {
        if (filename[i] == '.') {
            dot = &filename[i];
            break;
        }
        name_len++;
    }
    
    int copy_len = (name_len > 8) ? 8 : name_len;
    for (int i = 0; i < copy_len; i++) {
        fat_name[i] = filename[i];
    }

    if (dot) {
        int ext_len = strlen(dot + 1);
        int ext_copy = (ext_len > 3) ? 3 : ext_len;
        for (int i = 0; i < ext_copy; i++) {
            fat_name[8 + i] = dot[1 + i];
        }
    }
}

void fat16_83_to_filename(const char* fat_name, char* filename) {
    int pos = 0;

    for (int i = 0; i < 8; i++) {
        if (fat_name[i] != ' ') {
            filename[pos++] = fat_name[i];
        } else {
            break; 
        }
    }

    bool has_extension = false;
    for (int i = 8; i < 11; i++) {
        if (fat_name[i] != ' ') {
            has_extension = true;
            break;
        }
    }
    
    if (has_extension) {
        filename[pos++] = '.';
        for (int i = 8; i < 11; i++) {
            if (fat_name[i] != ' ') {
                filename[pos++] = fat_name[i];
            }
        }
    }
    
    filename[pos] = '\0';
}

static void fat16_create_dir_entry(fat16_dir_entry_t* entry, const char* filename, uint8_t attributes, uint16_t first_cluster, uint32_t file_size) {

    if (!entry || !filename){
        return;
    }
    memset(entry, 0, sizeof(fat16_dir_entry_t));
    char fat_name[12];
    fat16_filename_to_83(filename, fat_name);
    memcpy(entry->filename, fat_name, 11);

    entry->attributes = attributes;
    entry->first_cluster_low = first_cluster;
    entry->file_size = file_size;
    entry->creation_time = 0;
    entry->creation_date = 0;
    entry->last_write_time = 0;
    entry->last_write_date = 0;
    entry->last_access_date = 0;
}

static bool fat16_is_entry_empty(const fat16_dir_entry_t* entry) {
    return (entry->filename[0] == 0x00 || (unsigned char)entry->filename[0] == 0xE5);
}

static bool fat16_is_directory(const fat16_dir_entry_t* entry) {
    return (entry->attributes & FAT_ATTR_DIRECTORY) != 0;
}

static bool fat16_is_regular_file(const fat16_dir_entry_t* entry) {
    return !fat16_is_entry_empty(entry) && 
           !fat16_is_directory(entry) &&
           !(entry->attributes & FAT_ATTR_VOLUME_ID);
}

static void fat16_extract_file_info(const fat16_dir_entry_t* entry, fat16_file_info_t* info) {
    
    fat16_83_to_filename(entry->filename, info->filename);
    
    
    info->file_size = entry->file_size;
    info->first_cluster = entry->first_cluster_low;
    info->attributes = entry->attributes;
    
    
    info->is_directory = (entry->attributes & FAT_ATTR_DIRECTORY) != 0;
    info->is_readonly = (entry->attributes & FAT_ATTR_READ_ONLY) != 0;
    info->is_hidden = (entry->attributes & FAT_ATTR_HIDDEN) != 0;
    info->is_system = (entry->attributes & FAT_ATTR_SYSTEM) != 0;
}


bool fat16_get_file_info(const char* filename, fat16_file_info_t* info) {
    if (!fs_ctx.mounted || !filename || !info) {
        return false;
    }
    
    static fat16_dir_entry_t entries[64];
    int entry_count;

    if (!fat16_read_directory_cluster(dir_ctx.current_dir_cluster, entries, 64, &entry_count)) {
        return false;
    }
    
    for (int i = 0; i < entry_count; i++) {
        char entry_filename[13];
        fat16_83_to_filename(entries[i].filename, entry_filename);
        if (strcmp(entry_filename, filename) == 0) {
            fat16_extract_file_info(&entries[i], info);
            return true;
        }
    }
    
    return false;
}


bool fat16_set_file_attributes(const char* filename, uint8_t attributes __attribute__((unused))) {
    if (!fs_ctx.mounted || !filename) {
        return false;
    }
    

    
    return false;
}


uint32_t fat16_get_file_size(const char* filename) {
    if (!fs_ctx.mounted || !filename) {
        return 0;
    }
    
    fat16_file_info_t info;
    if (fat16_get_file_info(filename, &info)) {
        return info.file_size;
    }
    
    return 0; 
}


static bool fat16_is_valid_attributes(uint8_t attributes) {
    
    uint8_t valid_mask = FAT_ATTR_READ_ONLY | FAT_ATTR_HIDDEN | FAT_ATTR_SYSTEM | 
                        FAT_ATTR_VOLUME_ID | FAT_ATTR_DIRECTORY | FAT_ATTR_ARCHIVE;
    
    return (attributes & ~valid_mask) == 0;
}


bool fat16_read_sector(uint32_t sector, void* buffer) {
    if (!buffer) {
        return false;
    }
    return ata_read_sector(sector, buffer);
}


bool fat16_write_sector(uint32_t sector, const void* buffer) {
    if (!buffer) {
        return false;
    }
    return ata_write_sector(sector, buffer);
}


bool fat16_read_sectors(uint32_t start_sector, uint32_t count, void* buffer) {
    if (!buffer || count == 0) {
        return false;
    }
    
    uint8_t* buf_ptr = (uint8_t*)buffer;
    
    for (uint32_t i = 0; i < count; i++) {
        if (!fat16_read_sector(start_sector + i, buf_ptr)) {
            return false;
        }
        buf_ptr += 512; 
    }
    
    return true;
}


bool fat16_write_sectors(uint32_t start_sector, uint32_t count, const void* buffer) {
    if (!buffer || count == 0) {
        return false;
    }
    
    const uint8_t* buf_ptr = (const uint8_t*)buffer;
    
    for (uint32_t i = 0; i < count; i++) {
        if (!fat16_write_sector(start_sector + i, buf_ptr)) {
            return false;
        }
        buf_ptr += 512; 
    }
    
    return true;
}


bool fat16_read_file(const char* filename, void* buffer, uint32_t size) {
    if (!fs_ctx.mounted || !filename || !buffer || size == 0) {
        return false;
    }
    
    fat16_file_info_t file_info;
    if (!fat16_get_file_info(filename, &file_info)) {
        vga_printf("File not found: %s\n", filename);
        return false;
    }
    
    if (file_info.is_directory) {
        vga_printf("Cannot read directory as file: %s\n", filename);
        return false;
    }
    
    uint32_t read_size = (size > file_info.file_size) ? file_info.file_size : size;
    uint32_t bytes_read = 0;
    uint8_t* buf_ptr = (uint8_t*)buffer;
    uint16_t current_cluster = file_info.first_cluster;
    uint32_t cluster_size = fat16_get_cluster_size_bytes();
    
    while (bytes_read < read_size && fat16_is_valid_cluster(current_cluster)) {
        
        uint32_t bytes_to_read = read_size - bytes_read;
        if (bytes_to_read > cluster_size) {
            bytes_to_read = cluster_size;
        }
        
        uint32_t sector = fat16_cluster_to_sector(current_cluster);
        uint32_t sectors_to_read = fat16_get_cluster_size_sectors();
        
        if (!fat16_read_sectors(sector, sectors_to_read, buf_ptr)) {
            return false;
        }
        
        bytes_read += bytes_to_read;
        buf_ptr += bytes_to_read;
        
        current_cluster = fat16_get_next_cluster(current_cluster);
        if (current_cluster == FAT16_END_OF_CHAIN) {
            break;
        }
    }
    
    return true;
}


bool fat16_write_file(const char* filename, const void* buffer, uint32_t size) {
    if (!fs_ctx.mounted || !filename || !buffer || size == 0) {
        return false;
    }
    
    fat16_file_info_t file_info;
    bool file_exists = fat16_get_file_info(filename, &file_info);
    
    if (file_exists) {
        if (file_info.is_readonly) {
            vga_printf("Cannot write to read-only file: %s\n", filename);
            return false;
        }
    }

    uint16_t first_cluster = fat16_allocate_file_clusters(size);
    if (first_cluster == 0) {
        vga_printf("No space available for file: %s\n", filename);
        return false;
    }
    
    uint32_t bytes_written = 0;
    const uint8_t* buf_ptr = (const uint8_t*)buffer;
    uint16_t current_cluster = first_cluster;
    uint32_t cluster_size = fat16_get_cluster_size_bytes();
    
    while (bytes_written < size && fat16_is_valid_cluster(current_cluster)) {
        
        uint32_t bytes_to_write = size - bytes_written;
        if (bytes_to_write > cluster_size) {
            bytes_to_write = cluster_size;
        }
        
        uint32_t sector = fat16_cluster_to_sector(current_cluster);
        uint32_t sectors_to_write = fat16_get_cluster_size_sectors();
        
        if (!fat16_write_sectors(sector, sectors_to_write, buf_ptr)) {
            return false;
        }
        
        bytes_written += bytes_to_write;
        buf_ptr += bytes_to_write;
        
        current_cluster = fat16_get_next_cluster(current_cluster);
        if (current_cluster == FAT16_END_OF_CHAIN) {
            break;
        }
    }
    
    
    vga_printf("File written successfully: %s (%d bytes)\n", filename, size);
    return true;
}


bool fat16_create_file(const char* filename, uint8_t attributes) {
    if (!filename) {
        return false;
    }
    
    if (!fs_ctx.mounted) {
        vga_printf("Filesystem not mounted\n");
        return false;
    }
    
    if (!fat16_is_valid_attributes(attributes)) {
        vga_printf("Invalid file attributes\n");
        return false;
    }
    
    static fat16_dir_entry_t entries[64];
    int entry_count;

    if (!fat16_read_directory_cluster(dir_ctx.current_dir_cluster, entries, 64, &entry_count)) {
        vga_printf("Failed to read current directory\n");
        return false;
    }
    
    for (int i = 0; i < entry_count; i++) {
        char entry_name[13];
        fat16_83_to_filename(entries[i].filename, entry_name);
        
        if (strcmp(entry_name, filename) == 0) {
            if (attributes & FAT_ATTR_DIRECTORY) {
                vga_printf("Directory already exists: %s\n", filename);
            } else {
                vga_printf("File already exists: %s\n", filename);
            }
            return false;
        }
    }
    
    uint16_t first_cluster = 0;
    
    if (attributes & FAT_ATTR_DIRECTORY) {
        first_cluster = fat16_find_free_cluster();
        if (first_cluster == 0) {
            vga_printf("No free clusters available for directory\n");
            return false;
        }
        
        if (!fat16_allocate_cluster(first_cluster)) {
            vga_printf("Failed to allocate cluster for directory\n");
            return false;
        }
        
        if (!fat16_save_fat_table()) {
            vga_printf("Failed to save FAT table\n");
            return false;
        }
    }
    
    fat16_dir_entry_t new_entry;
    fat16_create_dir_entry(&new_entry, filename, attributes, first_cluster, 0);
    
    if (!fat16_write_directory_entry_to_cluster(&new_entry, dir_ctx.current_dir_cluster)) {
        vga_printf("Failed to write directory entry\n");
        return false;
    }
    
    vga_printf("File created successfully: %s\n", filename);
    return true;
}

bool fat16_delete_file(const char* filename) {
    if (!filename) {
        return false;
    }
    
    if (!fs_ctx.mounted) {
        vga_printf("Filesystem not mounted\n");
        return false;
    }
    
    static fat16_dir_entry_t entries[64];
    int entry_count = fat16_read_root_directory(entries, 64);
    
    uint16_t first_cluster = 0;
    bool file_found = false;
    
    for (int i = 0; i < entry_count; i++) {
        char entry_filename[13];
        fat16_83_to_filename(entries[i].filename, entry_filename);
        if (strcmp(entry_filename, filename) == 0) {
            if (entries[i].attributes & FAT_ATTR_DIRECTORY) {
                vga_printf("Cannot delete directory as file: %s\n", filename);
                return false;
            }
            
            first_cluster = entries[i].first_cluster_low;
            file_found = true;
            break;
        }
    }
    
    if (!file_found) {
        vga_printf("File not found: %s\n", filename);
        return false;
    }
    
    if (first_cluster >= 2 && fs_ctx.fat_table) {
        uint16_t current_cluster = first_cluster;
        int clusters_freed = 0;
        while (current_cluster >= 2 && current_cluster < 0xFFF8) {
            uint16_t next_cluster = fs_ctx.fat_table[current_cluster];
            
            fs_ctx.fat_table[current_cluster] = FAT16_FREE_CLUSTER;
            clusters_freed++;
            
            if (next_cluster >= 0xFFF8) {
                break; 
            }
            current_cluster = next_cluster;
        }
        
        if (!fat16_save_fat_table()) {
            vga_printf("Warning: Failed to save FAT table after freeing clusters\n");
        }
    }
    
    if (!fat16_delete_directory_entry(filename)) {
        vga_printf("Failed to delete directory entry: %s\n", filename);
        return false;
    }
    
    vga_printf("File deleted successfully: %s\n", filename);
    return true;
}




void fat16_set_current_directory(const char* path) {
    if (!path) {
        return;
    }
    
    strncpy(dir_ctx.current_path, path, sizeof(dir_ctx.current_path) - 1);
    dir_ctx.current_path[sizeof(dir_ctx.current_path) - 1] = '\0';
    
    
    dir_ctx.is_root = (strcmp(path, "/") == 0);
}


const char* fat16_get_current_directory(void) {
    return dir_ctx.current_path;
}


bool fat16_change_directory(const char* dirname) {
    if (!dirname) {
        return false;
    }
    
    if (strcmp(dirname, ".") == 0) {
        return true;
    }
    
    if (strcmp(dirname, "/") == 0) {
        fat16_set_current_directory("/");
        dir_ctx.current_dir_cluster = 0;
        dir_ctx.is_root = true;
        return true;
    }
    
    if (strcmp(dirname, "..") == 0) {
        if (dir_ctx.is_root) {
            return true; 
        }
        
        fat16_set_current_directory("/");
        dir_ctx.current_dir_cluster = 0;
        dir_ctx.is_root = true;
        return true;
    }
    
    static fat16_dir_entry_t entries[64];
    int entry_count;
    
    if (!fat16_read_directory_cluster(dir_ctx.current_dir_cluster, entries, 64, &entry_count)) {
        vga_printf("Failed to read current directory\n");
        return false;
    }
    
    for (int i = 0; i < entry_count; i++) {
        char entry_name[13];
        fat16_83_to_filename(entries[i].filename, entry_name);
        if (strcmp(entry_name, dirname) == 0) {
            if (entries[i].attributes & FAT_ATTR_DIRECTORY) {
                dir_ctx.current_dir_cluster = entries[i].first_cluster_low;
                dir_ctx.is_root = false;
                if (strcmp(dir_ctx.current_path, "/") == 0) {
                    strcpy(dir_ctx.current_path, "/");
                    strcat(dir_ctx.current_path, dirname);
                } else {
                    strcat(dir_ctx.current_path, "/");
                    strcat(dir_ctx.current_path, dirname);
                }
                
                return true;
            } else {
                vga_printf("'%s' is not a directory\n", dirname);
                return false;
            }
        }
    }
    
    vga_printf("Directory not found: %s\n", dirname);
    return false;
}


void fat16_test_basic_functions(void) {
    vga_printf_colored(VGA_COLOR_GREEN, VGA_COLOR_BLACK,
                      "=== Testing Basic FAT-16 Functions ===\n");
    
    
    vga_printf("Testing initialization... ");
    fat16_init();
    vga_printf_colored(VGA_COLOR_GREEN, VGA_COLOR_BLACK, "PASS\n");
    
    
    vga_printf("Testing mount (should fail)... ");
    if (!fat16_mount()) {
        vga_printf_colored(VGA_COLOR_GREEN, VGA_COLOR_BLACK, "PASS (expected failure)\n");
    } else {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK, "FAIL (unexpected success)\n");
    }
    
    
    vga_printf("Testing disk read... ");
    static uint8_t test_buffer[512];
    if (fat16_read_sector(0, test_buffer)) {
        vga_printf_colored(VGA_COLOR_GREEN, VGA_COLOR_BLACK, "PASS (sector 0 read)\n");
    } else {
        vga_printf_colored(VGA_COLOR_GREEN, VGA_COLOR_BLACK, "WARN (disk read failed)\n");
    }
    
    vga_printf("\n");
}


void fat16_test_cluster_operations(void) {
    vga_printf_colored(VGA_COLOR_GREEN, VGA_COLOR_BLACK,
                      "=== Testing Cluster Operations ===\n");
    
    
    fs_ctx.mounted = true;
    fs_ctx.total_clusters = 100;
    
    
    static uint16_t test_fat[102]; 
    fs_ctx.fat_table = test_fat;
    
    
    test_fat[0] = 0xFFF8; 
    test_fat[1] = 0xFFFF; 
    for (int i = 2; i < 102; i++) {
        test_fat[i] = FAT16_FREE_CLUSTER;
    }
    
    
    vga_printf("Testing find free cluster... ");
    uint16_t free_cluster = fat16_find_free_cluster();
    if (free_cluster == 2) {
        vga_printf_colored(VGA_COLOR_GREEN, VGA_COLOR_BLACK, "PASS (found cluster %d)\n", free_cluster);
    } else {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK, "FAIL (expected 2, got %d)\n", free_cluster);
    }
    
    vga_printf("\n");
}


void fat16_test_filename_conversion(void) {
    vga_printf_colored(VGA_COLOR_GREEN, VGA_COLOR_BLACK,
                      "=== Testing Filename Conversion ===\n");
    
    char fat_name[12];
    char filename[256];
    
    
    vga_printf("Testing 'test.txt' conversion... ");
    fat16_filename_to_83("test.txt", fat_name);
    fat_name[11] = '\0'; 
    
    
    if (fat_name[0] == 't' && fat_name[1] == 'e' && fat_name[2] == 's' && fat_name[3] == 't' &&
        fat_name[4] == ' ' && fat_name[8] == 't' && fat_name[9] == 'x' && fat_name[10] == 't') {
        vga_printf_colored(VGA_COLOR_GREEN, VGA_COLOR_BLACK, "PASS\n");
    } else {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK, "FAIL\n");
    }
    
    
    vga_printf("Testing reverse conversion... ");
    fat16_83_to_filename(fat_name, filename);
    if (strcmp(filename, "test.txt") == 0) {
        vga_printf_colored(VGA_COLOR_GREEN, VGA_COLOR_BLACK, "PASS\n");
    } else {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK, "FAIL (got '%s')\n", filename);
    }
    
    vga_printf("\n");
}


void fat16_test_file_operations(void) {
    vga_printf_colored(VGA_COLOR_GREEN, VGA_COLOR_BLACK,
                      "=== Testing File Operations ===\n");
    
    fs_ctx.mounted = true;
    fs_ctx.boot_sector.sectors_per_cluster = 1;
    fs_ctx.boot_sector.bytes_per_sector = 512;
    fs_ctx.data_start_sector = 100;
    
    
    vga_printf("Testing cluster to sector conversion... ");
    uint32_t sector = fat16_cluster_to_sector(2);
    if (sector == 100) { 
        vga_printf_colored(VGA_COLOR_GREEN, VGA_COLOR_BLACK, "PASS (cluster 2 -> sector %d)\n", sector);
    } else {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK, "FAIL (expected 100, got %d)\n", sector);
    }
    
    
    vga_printf("Testing directory entry creation... ");
    fat16_dir_entry_t test_entry;
    fat16_create_dir_entry(&test_entry, "test.txt", FAT_ATTR_ARCHIVE, 5, 1024);
    if (test_entry.first_cluster_low == 5 && test_entry.file_size == 1024 && 
        test_entry.attributes == FAT_ATTR_ARCHIVE) {
        vga_printf_colored(VGA_COLOR_GREEN, VGA_COLOR_BLACK, "PASS\n");
    } else {
        vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK, "FAIL\n");
    }
    
    vga_printf("\n");
}


void fat16_create_boot_sector(fat16_boot_sector_t* boot_sector) {
    memset(boot_sector, 0, sizeof(fat16_boot_sector_t));
    boot_sector->jmp_instr[0] = 0xEB;
    boot_sector->jmp_instr[1] = 0x3C;
    boot_sector->jmp_instr[2] = 0x90;
    memcpy(boot_sector->oem_name, "ACORNOS ", 8);

    boot_sector->bytes_per_sector = 512;
    boot_sector->sectors_per_cluster = 1;
    boot_sector->reserved_sectors = 1;
    boot_sector->fat_count = 2;
    boot_sector->root_entries = 512;
    boot_sector->total_sectors_16 = 8192;
    boot_sector->media_descriptor = 0xF8;
    boot_sector->sectors_per_fat = 32;
    boot_sector->sectors_per_track = 63;
    boot_sector->heads = 16;
    boot_sector->hidden_sectors = 0;
    boot_sector->total_sectors_32 = 0;
}

bool fat16_format_disk(void) {
    vga_printf("Formatting disk with FAT-16...\n");
    
    
    vga_printf("Creating boot sector...\n");
    fat16_boot_sector_t boot_sector;
    fat16_create_boot_sector(&boot_sector);
    
    vga_printf("Writing boot sector to sector 100...\n");
    
    if (!fat16_write_sector(100, &boot_sector)) {
        vga_printf("Failed to write boot sector\n");
        return false;
    }
    
    vga_printf("Boot sector written successfully\n");
    vga_printf("FAT-16 boot sector written to sector 100\n");
    vga_printf("Format operation completed\n");
    return true;
}


int fat16_read_root_directory(fat16_dir_entry_t* entries, int max_entries) {
    if (!fs_ctx.mounted || !entries) {
        return 0;
    }

    uint32_t root_dir_sectors = (fs_ctx.boot_sector.root_entries * 32) / fs_ctx.boot_sector.bytes_per_sector;
    uint32_t entries_per_sector = fs_ctx.boot_sector.bytes_per_sector / 32; 
    int total_entries = 0;
    uint8_t sector_buffer[512];
    
    for (uint32_t sector = 0; sector < root_dir_sectors && total_entries < max_entries; sector++) {
        if (!fat16_read_sector(fs_ctx.root_dir_start_sector + sector, sector_buffer)) {
            vga_printf("Failed to read root directory sector %d\n", sector);
            return total_entries;
        }
        fat16_dir_entry_t* sector_entries = (fat16_dir_entry_t*)sector_buffer;

        for (uint32_t i = 0; i < entries_per_sector && total_entries < max_entries; i++) {
            fat16_dir_entry_t* entry = &sector_entries[i];
            if (entry->filename[0] == 0x00) {
                return total_entries;
            }
            
            if ((unsigned char)entry->filename[0] == 0xE5) {
                continue;
            }
            
            if (entry->attributes & FAT_ATTR_VOLUME_ID) {
                continue;
            }
            
            memcpy(&entries[total_entries], entry, sizeof(fat16_dir_entry_t));
            total_entries++;
        }
    }
    
    return total_entries;
}


bool fat16_write_directory_entry_to_cluster(const fat16_dir_entry_t* entry, uint16_t dir_cluster) {
    if (!fs_ctx.mounted || !entry) {
        return false;
    }
    
    if (dir_cluster == 0) {
        return fat16_write_directory_entry(entry);
    }
    
    if (!fat16_is_valid_cluster(dir_cluster)) {
        vga_printf("Invalid directory cluster: %d\n", dir_cluster);
        return false;
    }
    
    uint32_t sector = fat16_cluster_to_sector(dir_cluster);
    uint8_t sector_buffer[512];
    
    if (!fat16_read_sector(sector, sector_buffer)) {
        vga_printf("Failed to read directory cluster %d\n", dir_cluster);
        return false;
    }
    
    fat16_dir_entry_t* sector_entries = (fat16_dir_entry_t*)sector_buffer;
    int entries_per_sector = 512 / sizeof(fat16_dir_entry_t);
    
    for (int i = 0; i < entries_per_sector; i++) {
        if (sector_entries[i].filename[0] == 0x00 || 
            (unsigned char)sector_entries[i].filename[0] == 0xE5) {
            memcpy(&sector_entries[i], entry, sizeof(fat16_dir_entry_t));
            if (!fat16_write_sector(sector, sector_buffer)) {
                vga_printf("Failed to write directory entry to cluster %d\n", dir_cluster);
                return false;
            }
            vga_printf("Directory entry written to cluster %d\n", dir_cluster);
            return true;
        }
    }
    vga_printf("Directory cluster %d is full\n", dir_cluster);
    return false;
}


bool fat16_delete_directory_entry(const char* filename) {
    if (!fs_ctx.mounted || !filename) {
        return false;
    }
    
    uint32_t root_dir_sectors = (fs_ctx.boot_sector.root_entries * 32) / fs_ctx.boot_sector.bytes_per_sector;
    uint32_t entries_per_sector = fs_ctx.boot_sector.bytes_per_sector / 32;
    uint8_t sector_buffer[512];
    
    for (uint32_t sector = 0; sector < root_dir_sectors; sector++) {
        if (!fat16_read_sector(fs_ctx.root_dir_start_sector + sector, sector_buffer)) {
            return false;
        }
        
        fat16_dir_entry_t* sector_entries = (fat16_dir_entry_t*)sector_buffer;
        
        for (uint32_t i = 0; i < entries_per_sector; i++) {
            fat16_dir_entry_t* entry = &sector_entries[i];
            
            
            if (entry->filename[0] == 0x00 || (unsigned char)entry->filename[0] == 0xE5) {
                continue;
            }
            
            char entry_filename[13];
            fat16_83_to_filename(entry->filename, entry_filename);
            
            if (strcmp(entry_filename, filename) == 0) {
                entry->filename[0] = (char)0xE5;
                if (!fat16_write_sector(fs_ctx.root_dir_start_sector + sector, sector_buffer)) {
                    return false;
                }
                vga_printf("Directory entry deleted: %s\n", filename);
                return true;
            }
        }
    }
    
    return false; 
}


bool fat16_read_file_content(const char* filename, void* buffer, uint32_t buffer_size, uint32_t* bytes_read) {
    if (!fs_ctx.mounted || !filename || !buffer || !bytes_read) {
        return false;
    }
    *bytes_read = 0;
    
    static fat16_dir_entry_t entries[64];
    int entry_count;

    if (!fat16_read_directory_cluster(dir_ctx.current_dir_cluster, entries, 64, &entry_count)) {
        vga_printf("Failed to read current directory\n");
        return false;
    }
    
    for (int i = 0; i < entry_count; i++) {
        char entry_filename[13];
        fat16_83_to_filename(entries[i].filename, entry_filename);
        
        if (strcmp(entry_filename, filename) == 0) {
            
            if (entries[i].attributes & FAT_ATTR_DIRECTORY) {
                vga_printf("Cannot read directory as file\n");
                return false;
            }
            
            uint32_t file_size = entries[i].file_size;
            uint16_t first_cluster = entries[i].first_cluster_low;
            
            if (file_size == 0 || first_cluster == 0) {
                *bytes_read = 0;
                return true;
            }
            
            uint32_t max_read = (file_size < buffer_size) ? file_size : buffer_size;
            if (fat16_read_cluster_chain(first_cluster, buffer, max_read, bytes_read)) {
                
                if (*bytes_read > file_size) {
                    *bytes_read = file_size;
                }
                return true;
            } else {
                vga_printf("Failed to read file content from clusters\n");
                return false;
            }
        }
    }
    
    vga_printf("File not found: %s\n", filename);
    return false; 
}


static bool fat16_write_cluster_chain(uint16_t* first_cluster, const void* buffer, uint32_t data_size) {
    if (!fs_ctx.mounted || !fs_ctx.fat_table || !buffer || !first_cluster) {
        return false;
    }
    
    const uint8_t* buf_ptr = (const uint8_t*)buffer;
    uint32_t cluster_size = fat16_get_cluster_size_bytes();
    uint32_t clusters_needed = (data_size + cluster_size - 1) / cluster_size;
    uint32_t bytes_written = 0;
    
    
    if (*first_cluster == 0) {
        *first_cluster = fat16_find_free_cluster();
        if (*first_cluster == 0) {
            vga_printf("No free clusters available\n");
            return false;
        }
    }
    
    uint16_t current_cluster = *first_cluster;
    uint8_t cluster_buffer[512];
    
    
    for (uint32_t cluster_num = 0; cluster_num < clusters_needed; cluster_num++) {
        
        memset(cluster_buffer, 0, sizeof(cluster_buffer));
        uint32_t bytes_to_write = cluster_size;
        if (bytes_written + bytes_to_write > data_size) {
            bytes_to_write = data_size - bytes_written;
        }
        
        memcpy(cluster_buffer, buf_ptr + bytes_written, bytes_to_write);
        
        
        uint32_t cluster_sector = fat16_cluster_to_sector(current_cluster);
        if (!fat16_write_sector(cluster_sector, cluster_buffer)) {
            vga_printf("Failed to write cluster %d\n", current_cluster);
            return false;
        }
        
        bytes_written += bytes_to_write;
        
        
        if (cluster_num < clusters_needed - 1) {
            
            uint16_t next_cluster = fat16_find_free_cluster();
            if (next_cluster == 0) {
                vga_printf("Out of disk space\n");
                return false;
            }
            
            fs_ctx.fat_table[current_cluster] = next_cluster;
            current_cluster = next_cluster;
        } else {
            
            fs_ctx.fat_table[current_cluster] = FAT16_END_OF_CHAIN;
        }
    }
    
    
    return fat16_save_fat_table();
}


bool fat16_write_file_content(const char* filename, const void* buffer, uint32_t data_size) {
    if (!fs_ctx.mounted || !filename || !buffer) {
        return false;
    }

    static fat16_dir_entry_t entries[64];
    int entry_count = fat16_read_root_directory(entries, 64);
    for (int i = 0; i < entry_count; i++) {
        char entry_filename[13];
        fat16_83_to_filename(entries[i].filename, entry_filename);
        
        if (strcmp(entry_filename, filename) == 0) {
            if (entries[i].attributes & FAT_ATTR_DIRECTORY) {
                vga_printf("Cannot write to directory\n");
                return false;
            }
    
            uint16_t first_cluster = entries[i].first_cluster_low;
            if (fat16_write_cluster_chain(&first_cluster, buffer, data_size)) {
                entries[i].first_cluster_low = first_cluster;
                entries[i].file_size = data_size;
                
                uint32_t entries_per_sector = fs_ctx.boot_sector.bytes_per_sector / 32;
                uint32_t sector_index = i / entries_per_sector;
                uint32_t entry_index = i % entries_per_sector;
                
                uint8_t sector_buffer[512];
                uint32_t sector_num = fs_ctx.root_dir_start_sector + sector_index;
                
                if (!fat16_read_sector(sector_num, sector_buffer)) {
                    vga_printf("Failed to read directory sector for update\n");
                    return false;
                }
                
                fat16_dir_entry_t* sector_entries = (fat16_dir_entry_t*)sector_buffer;
                memcpy(&sector_entries[entry_index], &entries[i], sizeof(fat16_dir_entry_t));
                
                if (!fat16_write_sector(sector_num, sector_buffer)) {
                    vga_printf("Failed to write updated directory entry\n");
                    return false;
                }
                
                vga_printf("File content written successfully\n");
                return true;
            } else {
                vga_printf("Failed to write file content\n");
                return false;
            }
        }
    }
    vga_printf("File not found: %s\n", filename);
    return false;
}

bool fat16_parse_path(const char* path, char components[][64], int* component_count) {
    if (!path || !components || !component_count) {
        return false;
    }
    
    *component_count = 0;
    
    const char* ptr = path;
    if (*ptr == '/') {
        ptr++; 
    }
    
    char temp_component[64];
    int temp_pos = 0;
    
    while (*ptr && *component_count < 16) { 
        if (*ptr == '/') {
            if (temp_pos > 0) {
                temp_component[temp_pos] = '\0';
                strcpy(components[*component_count], temp_component);
                (*component_count)++;
                temp_pos = 0;
            }
        } else if (temp_pos < 63) {
            temp_component[temp_pos++] = *ptr;
        }
        ptr++;
    }

    if (temp_pos > 0) {
        temp_component[temp_pos] = '\0';
        strcpy(components[*component_count], temp_component);
        (*component_count)++;
    }
    
    return true;
}

bool fat16_is_absolute_path(const char* path) {
    return path && path[0] == '/';
}

bool fat16_read_directory_cluster(uint16_t dir_cluster, fat16_dir_entry_t* entries, int max_entries, int* entry_count) {
    if (!entries || !entry_count) {
        return false;
    }
    
    *entry_count = 0;
    
    if (dir_cluster == 0) {
        *entry_count = fat16_read_root_directory(entries, max_entries);
        return *entry_count >= 0;
    }

    if (!fat16_is_valid_cluster(dir_cluster)) {
        return false;
    }
    
    uint32_t sector = fat16_cluster_to_sector(dir_cluster);
    uint8_t sector_buffer[512];
    
    if (!fat16_read_sector(sector, sector_buffer)) {
        vga_printf("Failed to read directory cluster %d\n", dir_cluster);
        return false;
    }
    
    fat16_dir_entry_t* sector_entries = (fat16_dir_entry_t*)sector_buffer;
    int entries_per_sector = 512 / sizeof(fat16_dir_entry_t);
    
    for (int i = 0; i < entries_per_sector && *entry_count < max_entries; i++) {
        fat16_dir_entry_t* entry = &sector_entries[i];
        
        if (entry->filename[0] == 0x00) {
            break;
        }
        
        if ((unsigned char)entry->filename[0] == 0xE5) {
            continue;
        }
        
        if (entry->attributes & FAT_ATTR_VOLUME_ID) {
            continue;
        }
        
        memcpy(&entries[*entry_count], entry, sizeof(fat16_dir_entry_t));
        (*entry_count)++;
    }
    
    return true;
}

uint16_t fat16_traverse_path(const char* path) {
    if (!path) {
        return 0;
    }
    
    char components[16][64];
    int component_count;
    
    if (!fat16_parse_path(path, components, &component_count)) {
        return 0;
    }
    
    uint16_t current_cluster = 0; 
    
    if (fat16_is_absolute_path(path)) {
        current_cluster = 0;
    } else {
        current_cluster = dir_ctx.current_dir_cluster;
    }
    
    for (int i = 0; i < component_count; i++) {
        const char* component = components[i];
    
        if (strcmp(component, ".") == 0) {
            continue; 
        }
        
        if (strcmp(component, "..") == 0) {
            if (current_cluster == 0) {
                continue; 
            }
            current_cluster = 0;
            continue;
        }
        
        static fat16_dir_entry_t entries[32];
        int entry_count;
        
        if (!fat16_read_directory_cluster(current_cluster, entries, 32, &entry_count)) {
            return 0; 
        }
        
        bool found = false;
        for (int j = 0; j < entry_count; j++) {
            char entry_name[13];
            fat16_83_to_filename(entries[j].filename, entry_name);
            
            if (strcmp(entry_name, component) == 0) {
                if (entries[j].attributes & FAT_ATTR_DIRECTORY) {
                    current_cluster = entries[j].first_cluster_low;
                    found = true;
                    break;
                } else {
                    return 0;
                }
            }
        }
        
        if (!found) {
            return 0; 
        }
    }
    
    return current_cluster;
}

bool fat16_create_subdirectory(const char* dirname) {
    if (!dirname || !fs_ctx.mounted) {
        return false;
    }
    
    if (!fat16_create_file(dirname, FAT_ATTR_DIRECTORY)) {
        return false;
    }
    
    static fat16_dir_entry_t entries[64];
    int entry_count;
    
    if (!fat16_read_directory_cluster(dir_ctx.current_dir_cluster, entries, 64, &entry_count)) {
        vga_printf("Failed to read current directory\n");
        return false;
    }
    
    uint16_t dir_cluster = 0;
    for (int i = 0; i < entry_count; i++) {
        char entry_name[13];
        fat16_83_to_filename(entries[i].filename, entry_name);
        
        if (strcmp(entry_name, dirname) == 0 && (entries[i].attributes & FAT_ATTR_DIRECTORY)) {
            dir_cluster = entries[i].first_cluster_low;
            break;
        }
    }
    
    if (dir_cluster == 0) {
        vga_printf("Directory entry found but no cluster allocated\n");
        return false;
    }
    
    fat16_dir_entry_t dot_entries[2];
    memset(dot_entries, 0, sizeof(dot_entries));
    memcpy(dot_entries[0].filename, ".          ", 11);
    dot_entries[0].attributes = FAT_ATTR_DIRECTORY;
    dot_entries[0].first_cluster_low = dir_cluster;
    dot_entries[0].file_size = 0;

    memcpy(dot_entries[1].filename, "..         ", 11);
    dot_entries[1].attributes = FAT_ATTR_DIRECTORY;
    dot_entries[1].first_cluster_low = dir_ctx.current_dir_cluster; 
    dot_entries[1].file_size = 0;
    
    uint32_t sector = fat16_cluster_to_sector(dir_cluster);
    uint8_t sector_buffer[512];
    memset(sector_buffer, 0, 512);
    
    memcpy(sector_buffer, dot_entries, sizeof(dot_entries));
    
    if (!fat16_write_sector(sector, sector_buffer)) {
        vga_printf("Failed to initialize directory entries\n");
        return false;
    }
    
    vga_printf("Directory created and initialized: %s\n", dirname);
    return true;
}

bool fat16_is_valid_cluster(uint16_t cluster) {
    if (cluster < 2) {
        return false; 
    }
    if (cluster >= 0xFFF8) {
        return false; 
    }
    if (cluster >= fs_ctx.total_clusters + 2) {
        return false; 
    }
    return true;
}

uint32_t fat16_cluster_to_sector(uint16_t cluster) {
    if (cluster < 2) {
        return 0; 
    }
    return fs_ctx.data_start_sector + ((cluster - 2) * fs_ctx.boot_sector.sectors_per_cluster);
}

bool fat16_write_directory_entry(const fat16_dir_entry_t* entry) {
    if (!fs_ctx.mounted || !entry) {
        return false;
    }
    
    uint32_t root_dir_sectors = (fs_ctx.boot_sector.root_entries * 32) / fs_ctx.boot_sector.bytes_per_sector;
    uint32_t entries_per_sector = fs_ctx.boot_sector.bytes_per_sector / 32;    
    uint8_t sector_buffer[512];

    for (uint32_t sector = 0; sector < root_dir_sectors; sector++) {
        if (!fat16_read_sector(fs_ctx.root_dir_start_sector + sector, sector_buffer)) {
            vga_printf("Failed to read root directory sector %d\n", sector);
            return false;
        }
        
        fat16_dir_entry_t* sector_entries = (fat16_dir_entry_t*)sector_buffer;
        
        for (uint32_t i = 0; i < entries_per_sector; i++) {
            if (sector_entries[i].filename[0] == 0x00 || 
                (unsigned char)sector_entries[i].filename[0] == 0xE5) {
                
                memcpy(&sector_entries[i], entry, sizeof(fat16_dir_entry_t));
                
                if (!fat16_write_sector(fs_ctx.root_dir_start_sector + sector, sector_buffer)) {
                    vga_printf("Failed to write directory entry to root\n");
                    return false;
                }
                
                vga_printf("Directory entry written successfully\n");
                return true;
            }
        }
    }
    
    vga_printf("Root directory is full\n");
    return false;
}

bool fat16_read_file_content_in_current_dir(const char* filename, void* buffer, uint32_t buffer_size, uint32_t* bytes_read) {
    if (!fs_ctx.mounted || !filename || !buffer || !bytes_read) {
        return false;
    }
    
    *bytes_read = 0;

    static fat16_dir_entry_t entries[64];
    int entry_count;
    
    if (!fat16_read_directory_cluster(dir_ctx.current_dir_cluster, entries, 64, &entry_count)) {
        vga_printf("Failed to read current directory for file search\n");
        return false;
    }
    
    for (int i = 0; i < entry_count; i++) {
        char entry_filename[13];
        fat16_83_to_filename(entries[i].filename, entry_filename);
        
        if (strcmp(entry_filename, filename) == 0) {
            if (entries[i].attributes & FAT_ATTR_DIRECTORY) {
                vga_printf("Cannot read directory as file\n");
                return false;
            }
            
            uint16_t first_cluster = entries[i].first_cluster_low;
            uint32_t file_size = entries[i].file_size;
            
            if (file_size == 0 || first_cluster == 0) {
                *bytes_read = 0;
                return true;
            }
            
            if (file_size > buffer_size) {
                vga_printf("File too large for buffer\n");
                return false;
            }
            
            return fat16_read_cluster_chain(first_cluster, buffer, buffer_size, bytes_read);
        }
    }
    
    vga_printf("File not found in current directory: %s\n", filename);
    return false;
}

bool fat16_write_file_content_in_current_dir(const char* filename, const void* buffer, uint32_t data_size) {
    if (!fs_ctx.mounted || !filename || !buffer) {
        return false;
    }

    static fat16_dir_entry_t entries[64];
    int entry_count;
    
    if (!fat16_read_directory_cluster(dir_ctx.current_dir_cluster, entries, 64, &entry_count)) {
        vga_printf("Failed to read current directory for file write\n");
        return false;
    }

    for (int i = 0; i < entry_count; i++) {
        char entry_filename[13];
        fat16_83_to_filename(entries[i].filename, entry_filename);
        
        if (strcmp(entry_filename, filename) == 0) {
            if (entries[i].attributes & FAT_ATTR_DIRECTORY) {
                vga_printf("Cannot write to directory\n");
                return false;
            }
    
            uint16_t first_cluster = entries[i].first_cluster_low;
            if (fat16_write_cluster_chain(&first_cluster, buffer, data_size)) {
                entries[i].first_cluster_low = first_cluster;
                entries[i].file_size = data_size;
            
                if (dir_ctx.current_dir_cluster == 0) {
                    return fat16_update_root_directory_entry(filename, data_size, first_cluster);
                } else {
                    uint32_t sector = fat16_cluster_to_sector(dir_ctx.current_dir_cluster);
                    uint8_t sector_buffer[512];
                    
                    if (!fat16_read_sector(sector, sector_buffer)) {
                        vga_printf("Failed to read directory sector for update\n");
                        return false;
                    }
                    
                    memcpy(sector_buffer, entries, sizeof(fat16_dir_entry_t) * entry_count);
    
                    if (!fat16_write_sector(sector, sector_buffer)) {
                        vga_printf("Failed to write updated directory entry\n");
                        return false;
                    }
                }
                vga_printf("File content written successfully\n");
                return true;
            } else {
                vga_printf("Failed to write file content\n");
                return false;
            }
        }
    }
    vga_printf("File not found: %s\n", filename);
    return false;
}