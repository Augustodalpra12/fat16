#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma pack(push, 1)
// Colocar o nome do arquivo na linha 70

typedef struct fat_BS
{
	unsigned char 		bootjmp[3];
	unsigned char 		oem_name[8];
	unsigned short 	        bytes_per_sector;
	unsigned char		sectors_per_cluster;
	unsigned short		reserved_sector_count;
	unsigned char		table_count;
	unsigned short		root_entry_count;
	unsigned short		total_sectors_16;
	unsigned char		media_type;
	unsigned short		table_size_16;
	unsigned short		sectors_per_track;
	unsigned short		head_side_count;
	unsigned int 		hidden_sector_count;
	unsigned int 		total_sectors_32;
 
	//this will be cast to it's specific type once the driver actually knows what type of FAT this is.
	unsigned char		extended_section[54];
 
} fat_BS_t;

typedef struct fat_RD
{
    unsigned char file_name[11];
    unsigned char atribute_of_file;
    unsigned char windows_nt;
    unsigned char creation_time;
	unsigned short time_file_created;
	unsigned short date_file_created;
	unsigned short last_accessed_date;
	unsigned short high_entry_fc;
	unsigned short last_modification_time;
	unsigned short last_modification_date;
	unsigned short low_entry_fc;
	unsigned int size_file_bytes;

} fat_RD_t;

typedef struct fat_table
{
	unsigned short next_fat;
} fat_table_1;

typedef struct file_data
{
	unsigned char char_data;
} file_data_t;

#pragma pack(pop)

int main()
{

	fat_BS_t boot_record;
	fat_RD_t root_directory;
	int entry_count = 0, size_RD_t = 1, fat_sequence_count = 2;
	fat_RD_t* root_directory_8_3 = (fat_RD_t*)malloc(size_RD_t * sizeof(fat_RD_t));
	fat_table_1* fat_1 = (fat_table_1*)malloc(fat_sequence_count * sizeof(fat_table_1));
	file_data_t *file_data = NULL;
    FILE *fp;  

    fp= fopen("./test.img", "rb");
    fseek(fp, 0, SEEK_SET);
    fread(&boot_record, sizeof(fat_BS_t),1, fp);

	printf("==== Dados do boot record ====\n");
    printf("Bytes per sector %hd \n", boot_record.bytes_per_sector);
	printf("Table Count %x \n", boot_record.table_count);
	printf("Number of reserved sectors %hd \n", boot_record.reserved_sector_count);
	printf("Number of root directory entries %hd \n", boot_record.root_entry_count);
	printf("Number of sectors per FAT %hd \n", boot_record.table_size_16);
    printf("Sector per cluster %x \n", boot_record.sectors_per_cluster);
	printf("\n==============================\n");

	// Calcular onde esta o root directory
	// bytes per sector * (setores reservados + (setores por fat * table_count))
    int int_bytes_per_sector = (int)boot_record.bytes_per_sector;
    int int_reserved_sector_count = (int)boot_record.reserved_sector_count;
    int int_table_size_16 = (int)boot_record.table_size_16;
    int int_table_count = (int)boot_record.table_count;
    int start_root_directory = int_bytes_per_sector * (int_reserved_sector_count + (int_table_size_16 * int_table_count));

    fseek(fp, start_root_directory, SEEK_SET);
	fread(&root_directory, sizeof(fat_RD_t),1, fp);
	int root_directory_iterator = start_root_directory;

	//Entradas 8.3
	printf("\n=== Entradas 8.3 ===\n");

	for(int i = 0; i < boot_record.root_entry_count; i++){
		if(root_directory.file_name[0] == 0xe5 || root_directory.atribute_of_file == 0x0f || root_directory.atribute_of_file == 0x00){
		root_directory_iterator += 32;	

		} else {
			// prints do root directory
			printf("Numero da entrada %d\n", entry_count);
			printf("File Name: ");
			for(int j = 0; j <= 7; j++){
				printf("%c", root_directory.file_name[j]);
			}
			printf("\n");
			printf("Extension: ");
			for(int j = 8; j <= 10; j++){
				printf("%c", root_directory.file_name[j]);
			}
			printf("\n");
			printf("Archive type: %x \n", root_directory.atribute_of_file);
			printf("First Cluster: %hd \n", root_directory.low_entry_fc);
			printf("Size File in Bytes: %d \n\n", root_directory.size_file_bytes);
			
			root_directory_8_3[entry_count] = root_directory;
			root_directory_iterator += 32;
			entry_count++;
			size_RD_t++;
			root_directory_8_3 = (fat_RD_t*)realloc(root_directory_8_3, size_RD_t * sizeof(fat_RD_t));

		}
		fseek(fp, root_directory_iterator, SEEK_SET);
		fread(&root_directory, sizeof(fat_RD_t),1, fp);
		
	}
	
	// localizando os dados
	int fat_1_place = boot_record.reserved_sector_count * boot_record.bytes_per_sector;
	char input[4];
	int file_chosen = 0;
	while(1){
		printf("\nQual arquivo deseja abrir? (numero). Digite SAIR para encerrar o programa\n");
		scanf("%s", input);

		if (strcmp(input, "SAIR") == 0) {
            printf("Programa finalizado.\n");
            break; 
        }
		sscanf(input, "%d", &file_chosen);
		unsigned short next_sector = 0x00;
		int first_cluster_place = fat_1_place + (root_directory_8_3[file_chosen].low_entry_fc * 2);
		fat_1[0].next_fat = root_directory_8_3[file_chosen].low_entry_fc;
		fseek(fp, first_cluster_place, SEEK_SET);
		fread(&fat_1[1].next_fat, sizeof(unsigned short),1, fp);
		first_cluster_place +=2;

		while(next_sector != 0xffff){
		if(fat_1[1].next_fat == 0xffff){
			break;
		}
		else{
			fseek(fp, first_cluster_place, SEEK_SET);
			fread(&fat_1[fat_sequence_count].next_fat, sizeof(unsigned short),1, fp);	
			next_sector = fat_1[fat_sequence_count].next_fat;	
			first_cluster_place += 2;
			fat_sequence_count++;
		}
		
		}

		// mostrando o conteudo do arquivo
		
		int first_data_place = (boot_record.reserved_sector_count + (boot_record.table_count * boot_record.table_size_16) + 32);
		int first_data_file = (first_data_place + ((root_directory_8_3[file_chosen].low_entry_fc - 2) * boot_record.sectors_per_cluster)) * boot_record.bytes_per_sector;
		int int_size_file_bytes = (int)root_directory_8_3[file_chosen].size_file_bytes;
		file_data_t *file_data = (file_data_t*)malloc(int_size_file_bytes * sizeof(file_data_t));

		fseek(fp, first_data_file, SEEK_SET);
		for(int i = 0; i < int_size_file_bytes; i++){
			fread(&file_data[i].char_data, sizeof(unsigned char),1, fp);
			printf("%c", file_data[i].char_data);
		}		
	}
	free(file_data);
	free(fat_1);
	free(root_directory_8_3);
    return 0;
}
