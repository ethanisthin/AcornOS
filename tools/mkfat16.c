/*
    mkfat16.c

    - This file is a utility used by AcornOS to help automate the process of formatting and mounting a drive.
    - Replaces the need to manually run the format and mount commands when OS initializes. 

*/

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

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
} boot_sector;

/*
    main()
    - this function parses the arguments, creates and writes to a boot sector.
    - it creates 2 copies of a FAT Table 
    - creates and writes the root directory (which is zeroed)
    - outputs to a file
*/
int main(int argc, char* argv[]){

    if (argc < 2){
        fprintf(stderr, "Usage: %s <output file>\n", argv[0]);
        return 1;
    }

    // matching the struct as defined in fat16 boot_sector_t 
    int bytes_per_sector   = 512;
    int sectors_per_cluster = 1;
    int reserved_sectors    = 1;
    int fat_count           = 2;
    int root_entries        = 512;
    int total_sectors       = 8192;
    int sectors_per_fat     = 32;
    int root_dir_sectors = (root_entries * 32) / bytes_per_sector;   
    int fat_start        = reserved_sectors;                          
    int root_dir_start   = fat_start + fat_count * sectors_per_fat;   
    int data_start       = root_dir_start + root_dir_sectors;  

    // boot sector creation process
    boot_sector boot;
    memset(&boot, 0, sizeof(boot));
    boot.jmp_instr[0] = 0xEB;
    boot.jmp_instr[1] = 0x3C;
    boot.jmp_instr[2] = 0x90;
    memcpy(boot.oem_name, "ACORNOS ", 8);
    boot.bytes_per_sector     = bytes_per_sector;
    boot.sectors_per_cluster  = sectors_per_cluster;
    boot.reserved_sectors     = reserved_sectors;
    boot.fat_count            = fat_count;
    boot.root_entries         = root_entries;
    boot.total_sectors_16     = total_sectors;
    boot.media_descriptor     = 0xF8;
    boot.sectors_per_fat      = sectors_per_fat;
    boot.sectors_per_track    = 63;
    boot.heads                = 16;
    boot.hidden_sectors       = 0;
    boot.total_sectors_32     = 0;

    int fat_size = sectors_per_fat*bytes_per_sector;
    uint16_t* fat = malloc(fat_size);
    memset(fat, 0, fat_size);
    fat[0] = 0xFFF8;
    fat[1] = 0xFFFF;

    int img_size = data_start*bytes_per_sector;
    uint8_t* img = calloc(1, img_size);

    memcpy(img + 0 * bytes_per_sector, &boot, sizeof(boot));
    memcpy(img + fat_start * bytes_per_sector,            fat, fat_size);
    memcpy(img + (fat_start + sectors_per_fat) * bytes_per_sector, fat, fat_size);

    FILE* fp = fopen(argv[1], "wb");
    if (!fp) {
        fprintf(stderr, "Failed to open %s for writing\n", argv[1]);
        free(fat);
        free(img);
        return 1;
    }

    fwrite(img, 1, img_size, fp);
    fclose(fp);
    printf("FAT16 img created: %s (%d bytes, %d sectors)\n", argv[1], img_size, img_size / bytes_per_sector);
    free(fat);
    free(img);

    return 0;
}