/* Libraries */
#include "fat12.h"
#include "../kernel/vga.h"

/* Function Declarations */
static void str_to_fat_name(const char* filename, char* fat_name);
static int fat_name_compare(const char* fat_name, const char* filename);
static void* memset(void* dest, int val, int n);

/* Global Variables */
static struct fat12_boot_sector boot_sector;
static uint8_t fat_table[SECTOR_SIZE * 2];
static uint32_t fat_start_sector;
static uint32_t root_dir_start_sector;
static uint32_t data_start_sector;
static uint8_t root_directory[SECTOR_SIZE * 14];
static int fs_initialized = 0;
static char file_store[10][512];
static int next_store_slot = 0;

static void* memcpy(void* dest, const void* src, int n) {
    char* d = (char*)dest;
    const char* s = (const char*)src;
    for (int i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dest;
}

static void* memset(void* dest, int val, int n) {
    char* d = (char*)dest;
    for (int i = 0; i < n; i++) {
        d[i] = val;
    }
    return dest;
}

int fat12_init(){
    boot_sector.bytes_per_sector = SECTOR_SIZE;
    boot_sector.sectors_per_cluster = 1;
    boot_sector.reserved_sectors = 1;
    boot_sector.fat_count = 2;
    boot_sector.root_entries = 224;
    boot_sector.total_sectors = 2880;
    boot_sector.media_descriptor = 0xF0;
    boot_sector.sectors_per_fat = 9;

    fat_start_sector = boot_sector.reserved_sectors;
    root_dir_start_sector = fat_start_sector + (boot_sector.fat_count * boot_sector.sectors_per_fat);
    data_start_sector = root_dir_start_sector + ((boot_sector.root_entries * 32) / boot_sector.bytes_per_sector);

    memset(fat_table, 0, SECTOR_SIZE * 2);
    fat_table[0] = 0xF0;
    fat_table[1] = 0xFF;
    fat_table[2] = 0xFF;

    memset(root_directory, 0, sizeof(root_directory));
    
    //test file
    struct fat12_dir_entry* test_entry = (struct fat12_dir_entry*)root_directory;
    memcpy(test_entry->filename, "WELCOME ", 8);
    memcpy(test_entry->extension, "TXT", 3);
    test_entry->attributes = FAT12_ATTR_ARCHIVE;
    test_entry->cluster_low = 2;
    test_entry->file_size = 25;
    const char* welcome_text = "Welcome to AcornOS v0.1!";
    memcpy(file_store[0], welcome_text, 25);
    next_store_slot = 1;
    
    fat12_set_next_cluster(2, FAT12_EOF_CLUSTER);
    
    fs_initialized = 1;
    return 0;
}

int fat12_read_sector(uint32_t sector, void* buffer) {
    if (sector >= root_dir_start_sector && 
        sector < root_dir_start_sector + ((boot_sector.root_entries * 32) / SECTOR_SIZE)) {
        uint32_t offset = (sector - root_dir_start_sector) * SECTOR_SIZE;
        memcpy(buffer, root_directory + offset, SECTOR_SIZE);
        return 0;
    }
    
    memset(buffer, 0, SECTOR_SIZE);
    return 0;
}

int fat12_write_sector(uint32_t sector, void* buffer){
    if (sector >= root_dir_start_sector && 
        sector < root_dir_start_sector + ((boot_sector.root_entries * 32) / SECTOR_SIZE)) {
        uint32_t offset = (sector - root_dir_start_sector) * SECTOR_SIZE;
        memcpy(root_directory + offset, buffer, SECTOR_SIZE);
        return 0;
    }
    return 0;
}

uint16_t fat12_get_next_cluster(uint16_t cluster){
    if (cluster >= FAT12_ENTRIES){
        return FAT12_EOF_CLUSTER;
    }

    uint32_t fat_offset = cluster + (cluster/2);
    uint16_t nxt_cluster;
    if (cluster % 2 == 0){
        nxt_cluster = fat_table[fat_offset] | ((fat_table[fat_offset+1] & 0x0f) << 8);
    } else {
        nxt_cluster = (fat_table[fat_offset] >> 4) | (fat_table[fat_offset+1] << 4);
    }
    return nxt_cluster;
}

int fat12_set_next_cluster(uint16_t cluster, uint16_t next){
    if (cluster >= FAT12_ENTRIES){
        return -1;
    }
    uint32_t fat_offset = cluster + (cluster/2);
    if (cluster % 2 == 0){
        fat_table[fat_offset] = next & 0xFF;
        fat_table[fat_offset + 1] = (fat_table[fat_offset+1]&0xF0) | ((next >> 8) & 0x0F);
    } else {
        fat_table[fat_offset] = (fat_table[fat_offset] & 0x0F) | ((next & 0x0F) << 4);
        fat_table[fat_offset + 1] = (next >> 4) & 0xFF;
    }
    return 0;
}

uint16_t fat12_find_free_cluster() {
    for (uint16_t cluster = 2; cluster < FAT12_ENTRIES; cluster++) {
        if (fat12_get_next_cluster(cluster) == FAT12_FREE_CLUSTER) {
            return cluster;
        }
    }
    return 0;
}

static void str_to_fat_name(const char* filename, char* fat_name) {
    int i = 0, j = 0;
    
    for (int k = 0; k < 11; k++) {
        fat_name[k] = ' ';
    }
    
    while (filename[i] && filename[i] != '.' && j < 8) {
        if (filename[i] >= 'a' && filename[i] <= 'z') {
            fat_name[j] = filename[i] - 'a' + 'A';
        } else {
            fat_name[j] = filename[i];
        }
        i++;
        j++;
    }
    
    if (filename[i] == '.') {
        i++;
    }
    
    j = 8;
    while (filename[i] && j < 11) {
        if (filename[i] >= 'a' && filename[i] <= 'z') {
            fat_name[j] = filename[i] - 'a' + 'A';
        } else {
            fat_name[j] = filename[i];
        }
        i++;
        j++;
    }
}

static int fat_name_compare(const char* fat_name, const char* filename) {
    char converted_name[11];
    str_to_fat_name(filename, converted_name);
    
    for (int i = 0; i < 11; i++) {
        if (fat_name[i] != converted_name[i]) {
            return 0; 
        }
    }
    return 1; 
}

int fat12_create_file(const char* name, uint8_t attributes) {
    if (!fs_initialized) {
        return -1;
    }
    struct fat12_dir_entry* entries = (struct fat12_dir_entry*)root_directory;
    for (int i = 0; i < boot_sector.root_entries; i++) {
        if (entries[i].filename[0] == 0x00 || entries[i].filename[0] == 0xE5) {

            memset(&entries[i], 0, sizeof(struct fat12_dir_entry));
            str_to_fat_name(name, entries[i].filename);
            entries[i].attributes = attributes;
            entries[i].cluster_low = 0; 
            entries[i].file_size = 0;
            return 0;
        }
    }
    return -1;
}

int fat12_delete_file(const char* name) {
    if (!fs_initialized) {
        return -1;
    }
    struct fat12_dir_entry* entries = (struct fat12_dir_entry*)root_directory;
    for (int i = 0; i < boot_sector.root_entries; i++) {
        if (fat_name_compare(entries[i].filename, name)) {
            entries[i].filename[0] = 0xE5;
            uint16_t cluster = entries[i].cluster_low;
            while (cluster >= 2 && cluster < 0xFF8) {
                uint16_t next = fat12_get_next_cluster(cluster);
                fat12_set_next_cluster(cluster, FAT12_FREE_CLUSTER);
                cluster = next;
            }
            return 0;
        }
    }
    return -1;
}

int fat12_read_file(const char* name, void* buffer, uint32_t size) {
    if (!fs_initialized) {
        return -1;
    }
    struct fat12_dir_entry entry;
    uint8_t sector_buffer[SECTOR_SIZE];

    for (uint32_t sector = 0; sector < (boot_sector.root_entries*32)/SECTOR_SIZE; sector++) {
        fat12_read_sector(root_dir_start_sector + sector, sector_buffer);
        
        struct fat12_dir_entry* dir_entry = (struct fat12_dir_entry*)sector_buffer;
        for (int i = 0; i < SECTOR_SIZE/sizeof(struct fat12_dir_entry); i++) {
            if (fat_name_compare(dir_entry[i].filename, name)) {
                memcpy(&entry, &dir_entry[i], sizeof(struct fat12_dir_entry));
                goto found;
            }
        }
    }
    return -1;
    
found:
    if (entry.cluster_low == 0 || entry.file_size == 0) {
        return 0;
    }
    
    if (entry.cluster_low >= 2 && entry.cluster_low < 12) {
        int storage_index = entry.cluster_low - 2;
        int bytes_to_read = entry.file_size;
        if (bytes_to_read > size) bytes_to_read = size;
        
        memcpy(buffer, file_store[storage_index], bytes_to_read);
        return bytes_to_read;
    }
    
    return 0;
}

int fat12_write_file(const char* name, void* buffer, uint32_t size) {
    if (!fs_initialized) {
        return -1;
    }
    
    struct fat12_dir_entry* entries = (struct fat12_dir_entry*)root_directory;
    int file_index = -1;
    
    for (int i = 0; i < boot_sector.root_entries; i++) {
        if (fat_name_compare(entries[i].filename, name)) {
            file_index = i;
            break;
        }
    }
    
    if (file_index == -1) {
        for (int i = 0; i < boot_sector.root_entries; i++) {
            if (entries[i].filename[0] == 0x00 || entries[i].filename[0] == 0xE5) {
                file_index = i;
                memset(&entries[i], 0, sizeof(struct fat12_dir_entry));
                str_to_fat_name(name, entries[i].filename);
                entries[i].attributes = FAT12_ATTR_ARCHIVE;
                break;
            }
        }
    }
    
    if (file_index == -1) {
        return -1; 
    }
    
    if (entries[file_index].cluster_low == 0) {
        if (next_store_slot >= 10) {
            return -1;
        }
        entries[file_index].cluster_low = next_store_slot + 2;
        fat12_set_next_cluster(entries[file_index].cluster_low, FAT12_EOF_CLUSTER);
        next_store_slot++;
    }
    
    int storage_index = entries[file_index].cluster_low - 2;
    int bytes_to_write = size;
    if (bytes_to_write > 512) bytes_to_write = 512;
    
    memcpy(file_store[storage_index], buffer, bytes_to_write);
    entries[file_index].file_size = bytes_to_write;
    
    return bytes_to_write;
}

int fat12_list_directory(struct fat12_dir_entry* entries, int max_entries) {
    if (!fs_initialized) return -1;
    
    uint8_t sector_buffer[SECTOR_SIZE];
    int entries_found = 0;
    
    for (uint32_t sector = 0; sector < (boot_sector.root_entries*32)/SECTOR_SIZE; sector++) {
        if (fat12_read_sector(root_dir_start_sector + sector, sector_buffer) != 0) {
            return -1;
        }
        
        struct fat12_dir_entry* dir_entry = (struct fat12_dir_entry*)sector_buffer;
        for (int i = 0; i < SECTOR_SIZE/sizeof(struct fat12_dir_entry); i++) {
            if (entries_found >= max_entries) {
                return entries_found;
            }
            if (dir_entry[i].filename[0] == 0x00) {
                continue;
            }
            if (dir_entry[i].filename[0] == 0xE5) {
                continue;
            }

            memcpy(&entries[entries_found], &dir_entry[i], sizeof(struct fat12_dir_entry));
            entries_found++;
        }
    }
    
    return entries_found;
}

int fat12_is_initialized() {
    return fs_initialized;
}