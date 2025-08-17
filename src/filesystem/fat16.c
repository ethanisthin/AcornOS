#include "fat16.h"
#include "../drivers/vga.h"
#include "../lib/string/string.h"
#include "../drivers/ata.h"
#include <stdbool.h>


static fat16_context_t fs_ctx;
static fat16_dir_context_t dir_ctx;

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
    
    
    
    
    
    
    vga_printf_colored(VGA_COLOR_RED, VGA_COLOR_BLACK,
                      "Mount not implemented yet\n");
    return false;
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

    for (uint32_t i = 2; i < fs_ctx.total_clusters + 2; i++) {
        if (fs_ctx.fat_table[i] == FAT16_FREE_CLUSTER) {
            return (uint16_t)i;
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

static bool fat16_free_cluster(uint16_t cluster) {
    if (!fs_ctx.mounted || !fs_ctx.fat_table || cluster < 2) {
        return false;
    }
    
    if (cluster >= fs_ctx.total_clusters + 2) {
        return false;
    }
    
    fs_ctx.fat_table[cluster] = FAT16_FREE_CLUSTER;
    return true;
}

static uint32_t fat16_cluster_to_sector(uint16_t cluster) {
    if (cluster < 2) {
        return 0; 
    }

    return fs_ctx.data_start_sector + ((cluster - 2) * fs_ctx.boot_sector.sectors_per_cluster);
}

static uint32_t fat16_get_cluster_size_sectors(void) {
    return fs_ctx.boot_sector.sectors_per_cluster;
}

static uint32_t fat16_get_cluster_size_bytes(void) {
    return fs_ctx.boot_sector.sectors_per_cluster * fs_ctx.boot_sector.bytes_per_sector;
}

static bool fat16_is_valid_cluster(uint16_t cluster) {
    return (cluster >= 2 && cluster < fs_ctx.total_clusters + 2);
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

static void fat16_filename_to_83(const char* filename, char* fat_name) {
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

static void fat16_83_to_filename(const char* fat_name, char* filename) {
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

static void fat16_create_dir_entry(fat16_dir_entry_t* entry, const char* filename, 
                                   uint8_t attributes, uint16_t first_cluster, uint32_t file_size) {
    memset(entry, 0, sizeof(fat16_dir_entry_t));
    fat16_filename_to_83(filename, entry->filename);
    
    entry->attributes = attributes;
    entry->first_cluster_low = first_cluster;
    entry->first_cluster_high = 0;
    entry->file_size = file_size;
    
    entry->creation_time = 0;
    entry->creation_date = 0;
    entry->last_write_time = 0;
    entry->last_write_date = 0;
    entry->last_access_date = 0;
}

static bool fat16_is_entry_empty(const fat16_dir_entry_t* entry) {
    return (entry->filename[0] == 0x00 || entry->filename[0] == 0xE5);
}

static bool fat16_is_directory(const fat16_dir_entry_t* entry) {
    return (entry->attributes & FAT_ATTR_DIRECTORY) != 0;
}

static bool fat16_is_regular_file(const fat16_dir_entry_t* entry) {
    return !fat16_is_entry_empty(entry) && 
           !fat16_is_directory(entry) &&
           !(entry->attributes & FAT_ATTR_VOLUME_ID);
}

static bool fat16_filename_matches(const fat16_dir_entry_t* entry, const char* filename) {
    char fat_name[12];
    fat16_filename_to_83(filename, fat_name);
    
    
    for (int i = 0; i < 11; i++) {
        if (fat_name[i] != entry->filename[i]) {
            return false;
        }
    }
    
    return true;
}


bool fat16_get_file_info(const char* filename, fat16_file_info_t* info) {
    if (!fs_ctx.mounted || !filename || !info) {
        return false;
    }
    
    
    
    return false;
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


bool fat16_set_file_attributes(const char* filename, uint8_t attributes) {
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
    
    if ((attributes & FAT_ATTR_VOLUME_ID) && (attributes & FAT_ATTR_DIRECTORY)) {
        return false; 
    }
    
    return true;
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
    
    
    if (!fat16_is_valid_attributes(attributes)) {
        vga_printf("Invalid file attributes\n");
        return false;
    }
    
    
    vga_printf("Creating file: %s (simulated)\n", filename);
    
    
    fat16_dir_entry_t new_entry;
    fat16_create_dir_entry(&new_entry, filename, attributes, 0, 0);
    
    
    
    
    return true; 
}


bool fat16_delete_file(const char* filename) {
    if (!filename) {
        return false;
    }
    
    
    vga_printf("Deleting file: %s (simulated)\n", filename);
    
    
    
    
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
    
    if (strcmp(dirname, "..") == 0) {
        
        if (dir_ctx.is_root) {
            return true; 
        }
        
        
        char* last_slash = NULL;
        for (int i = strlen(dir_ctx.current_path) - 1; i >= 0; i--) {
            if (dir_ctx.current_path[i] == '/') {
                last_slash = &dir_ctx.current_path[i];
                break;
            }
        }
        
        if (last_slash && last_slash != dir_ctx.current_path) {
            *last_slash = '\0'; 
        } else {
            strcpy(dir_ctx.current_path, "/"); 
        }
        
        dir_ctx.is_root = (strcmp(dir_ctx.current_path, "/") == 0);
        return true;
    }
    
    if (strcmp(dirname, "/") == 0) {
        
        fat16_set_current_directory("/");
        return true;
    }
    
    
    if (strlen(dirname) > 0 && dirname[0] != '/') {
        
        if (!dir_ctx.is_root) {
            strcat(dir_ctx.current_path, "/");
        }
        strcat(dir_ctx.current_path, dirname);
        dir_ctx.is_root = false;
        return true;
    }
    
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