#include "woody_woodpacker.h"

bool is_elf(int stream_input)
{
    char magic[4];

    if (read(stream_input, magic, 4) != 4)
    {
        printf("Error: could not read magic\n");
        perror("read");
        return false;
    }
    //Checker aussi que ce soit bien du elf 64 bits
    lseek(stream_input, 0, SEEK_SET);
    return magic[0] == 0x7f && magic[1] == 'E' && magic[2] == 'L' && magic[3] == 'F';
}

bool    write_woody(int stream_output) {
    // ARM Linux shellcode to write "Woody" to stdout
    uint8_t shellcode[] = {
        0x01, 0x00, 0xa0, 0xe3,  // mov r0, #1 (stdout fd)
        0x18, 0x10, 0x8f, 0xe2,  // add r1, pc, #24 (address of "Woody")
        0x05, 0x20, 0xa0, 0xe3,  // mov r2, #5 (length)
        0x04, 0x70, 0xa0, 0xe3,  // mov r7, #4 (write syscall)
        0x00, 0x00, 0x00, 0xef,  // svc 0 (syscall)
        0x00, 0x00, 0xa0, 0xe3,  // mov r0, #0 (exit code)
        0x01, 0x70, 0xa0, 0xe3,  // mov r7, #1 (exit syscall)
        0x00, 0x00, 0x00, 0xef,  // svc 0 (syscall)
        0x57, 0x6f, 0x6f, 0x64, 0x79  // "Woody" in ASCII
    };

    // Ajouter le jump sur l'entrypoint de base dans le shellcode.

    // Write shellcode to file
    int res = write(stream_output, shellcode, sizeof(shellcode));
    if (res <= 0) {
        return false;
    }
    return true;
}

bool copy_data(int stream_input, int stream_output)
{
    char buffer[4096];
    int bytes_read;

    while ((bytes_read = read(stream_input, buffer, 4096)) > 0)
    {
        if (write(stream_output, buffer, bytes_read) != bytes_read)
        {
            printf("Error: could not write to woody\n");
            perror("write");
            return false;
        }
    }
    if (bytes_read == -1)
    {
        perror("read");
        return false;
    }
    return true;
}

bool    get_len_file(int stream, int *len) {

    lseek(stream, 0, SEEK_SET);
    char buffer[4096];
    int bytes_read;

    while ((bytes_read = read(stream, buffer, 4096)) > 0)
    {
        *len += bytes_read;
    }
    if (bytes_read == -1)
    {
        perror("read");
        return false;
    }
    return true;
}

bool    replace_value(int stream, int value, int offset) {
    int err = lseek(stream, offset, SEEK_SET);
    if (err == (off_t)-1) {
        perror("Error seeking in file");
        return false;
    }
    write(stream, &value, sizeof(value));
    return true;
}

bool load_info(int stream, unsigned int offset, int len, char (*info)[8]) {
    int err = lseek(stream, offset, SEEK_SET);
    if (err == (off_t)-1) {
        perror("Error seeking in file");
        return false;
    }
    if (read(stream, *info, len) < 0)
    {
        perror("read");
        return false;
    }
    return true;
}

uint64_t convert_data_to_int(char *data, int bytes) {
    uint64_t value = 0;
    for (int i = 0; i < bytes; i++) {
        value |= ((uint64_t)(unsigned char)data[i] << (i * 8));
    }
    return value;
}

uint64_t convert_data_to_int_big_endian(char *data, int bytes) {
    uint64_t value = 0;
    for (int i = 0; i < bytes; i++) {
        // For big-endian, the first byte is the most significant
        value |= ((uint64_t)(unsigned char)data[i] << ((bytes - 1 - i) * 8));
    }
    return value;
}

bool get_shoff(int stream, int *shoff) {
    char data[8];
    if (!load_info(stream, 40, 8, &data))
    {
        return false;
    }
    *shoff = convert_data_to_int(data, 8);
    return true;
}

bool get_shnum(int stream, int *shnum) {
    char data[8];
    if (!load_info(stream, 60, 2, &data))
    {
        return false;
    }
    *shnum = convert_data_to_int(data, 2);
    return true;
}

bool    get_phoff(int stream, int *phoff) {
    char data[8];
    if (!load_info(stream, 32, 8, &data))
    {
        return false;
    }
    *phoff = convert_data_to_int(data, 8);
    return true;
}

bool    get_phnum(int stream, int *phnum) {
    char data[8];
    if (!load_info(stream, 56, 2, &data))
    {
        return false;
    }
    *phnum = convert_data_to_int(data, 2);
    return true;
}

bool    get_phentsize(int stream, int *phentsize) {
    char data[8];
    if (!load_info(stream, 54, 2, &data))
    {
        return false;
    }
    *phentsize = convert_data_to_int(data, 2);
    return true;
}

// allocate in the fonction
bool store_data(int stream, int offset, int len, char **data) {
    int err = lseek(stream, offset, SEEK_SET);
    if (err == (off_t)-1) {
        perror("Error seeking in file");
        return false;
    }
    *data = malloc(len);
    if (*data == NULL)
    {
        perror("malloc");
        return false;
    }
    if (read(stream, *data, len) < 0)
    {
        perror("read");
        return false;
    }
    return true;
}

bool    write_data(int stream, int len, char *data) {
    if (write(stream, data, len) < 0)
    {
        perror("write");
        return false;
    }
    return true;
}

bool    modify_entrypoints_ph_headers(int stream, int size_new_phdr /* should be always sizeof(elf64_phdr)*/, int phoff, int phnum) {
    int i = 0;
    while (i < phnum) {
        // load info a offset
        char data[8];
        // + 8 pour recuper la valeur de l'entrypoint
        int offset = phoff + (i * sizeof(Elf64_Phdr)) + 8;
        if (!load_info(stream, offset, 8, &data))
        {
            return false;
        }
        uint64_t entrypoint = convert_data_to_int(data, 8);
	    for (int i = 0; i < 8; i++) {
        	printf("%02X ", (unsigned char)(data)[i]);
    	}
	    printf("\n");
        printf("entrypoint: %llu, size_new_ph %d base %i offset %i\n", entrypoint, size_new_phdr, data[0], offset);

        if (!replace_value(stream, entrypoint + size_new_phdr, offset)) {
            return false;
        }
        i++;
    }
    return true;
}

bool    modify_entrypoints_section_headers(int stream, int size_new_phdr /* should be always sizeof(elf64_phdr)*/, int shoff, int shnum) {
    int i = 0;

    // charger a + 32 pour modifier l'offset du debut de la section en raw
    while (i < shnum) {
        // load info a offset
        char data[8];
        // + 8 pour recuper la valeur de l'entrypoint
        int offset = shoff + (i * sizeof(Elf64_Shdr)) + 24;
        if (!load_info(stream, offset, 8, &data))
        {
            return false;
        }
        uint64_t entrypoint = convert_data_to_int(data, 8);
        printf("entrypoint: %llu\n", entrypoint);

        if (!replace_value(stream, entrypoint + size_new_phdr, offset)) {
            return false;
        }
        i++;
    }
    return true;
}

bool    insert_new_phdr(int stream, size_t original_len, size_t added_bytes) {
    int phoff = 0;
    if (!get_phoff(stream, &phoff)) {
        return false;
    }
    int phnum = 0;
    if (!get_phnum(stream, &phnum)) {
        return false;
    }
    int shoff = 0;
    if (!get_shoff(stream, &shoff)) {
        return false;
    }
    int shnum = 0;
    if (!get_shnum(stream, &shnum)) {
        return false;
    }
    int phentsize = 0;
    if (!get_phentsize(stream, &phentsize)) {
        return false;
    }
    int offset_new_phdr = phoff + (phnum * phentsize);
    Elf64_Phdr *program_header;
    if (!create_program_header(&program_header, original_len, added_bytes)) {
        printf("Error: could not create program header\n");
        return 1;
    };

    if (!modify_entrypoints_ph_headers(stream, sizeof(Elf64_Phdr), phoff, phnum)) {
        printf("Error: could not modify entrypoints\n");
        return 1;
    };


    if (!insert_data(stream, offset_new_phdr, sizeof(Elf64_Phdr), (char *)program_header)) {
        printf("Error: could not insert program header\n");
        return 1;
    };
    // Modify the section header offset in the ELF header
    if (!replace_value(stream, shoff + sizeof(Elf64_Phdr), 40)) {
        printf("Error: could not update section header offset\n");
        return 1;
    }
    // Modify section header table entries
    if (!modify_entrypoints_section_headers(stream, sizeof(Elf64_Phdr), shoff + sizeof(Elf64_Phdr), shnum)) {
        printf("Error: could not modify section header entrypoints\n");
        return 1;
    };

    return true;
}

bool    insert_data(int stream, int offset, int len, char *data) {
    char *previous_data;
    if (!store_data(stream, offset, len, &previous_data)) {
        return false;
    }
    int err = lseek(stream, offset, SEEK_SET);
    if (err == (off_t)-1) {
        perror("Error seeking in file");
        return false;
    }
    if (write(stream, data, len) < 0)
    {
        perror("write");
        return false;
    }
    if (!write_data(stream, len, previous_data)) {
        return false;
    }
    return true;
}

bool    create_program_header(Elf64_Phdr **program_header, int offset, int len) {
    *program_header = malloc(sizeof(Elf64_Phdr));
    if (*program_header == NULL)
    {
        perror("malloc");
        return false;
    }
    Elf64_Phdr *pg_hdr = *program_header;
    pg_hdr->p_type = PT_LOAD;
    pg_hdr->p_flags = PF_R | PF_X;
    pg_hdr->p_offset = offset;
    pg_hdr->p_vaddr = 0x0000000000020018;
    pg_hdr->p_align = 0x10000;
    pg_hdr->p_paddr = 0x0000000000020018;
    pg_hdr->p_filesz = len;
    pg_hdr->p_memsz = len;

    return true;
}

bool increment_program_header(int stream) {
    char data[8];
    if (!load_info(stream, 56, 2, &data))
    {
        return false;
    }
    int phnum = convert_data_to_int(data, 2);
    phnum++;
    if (!replace_value(stream, phnum, 56)) {
        return false;
    }
    return true;
}


// ELFDATA2LSB little endian
//
// should check for lsb or msb big endian 
// and set a flag for converting data
int main(int ac, char **av)
{
    int stream_input;
    int stream_output;

    if (ac != 2)
    {
        printf("Usage: %s <binary>\n", av[0]);
        return 1;
    }
    if ((stream_input = open(av[1], O_RDONLY)) == -1)
    {
        printf("Error: could not open %s\n", av[1]);
        perror("open");
        return 1;
    }
    if (is_elf(stream_input))
    {
        if ((stream_output = open("./woody", O_CREAT | O_RDWR, 0755)) == -1)
        {
            printf("Error: could not open woody\n");
            perror("open");
            return 1;
        }
        if (!copy_data(stream_input, stream_output))
        {
            return 1;
        }
    } else {
        printf("Error: %s is not an ELF file\n", av[1]);
        return 1;
    }
//     ajouter le segment a la fin
    write_woody(stream_output);

    int len1 = 0;
    if (!get_len_file(stream_input, &len1)) {
        close(stream_input);
        close(stream_output);
        return 1;
    };
    int len2 = 0;
    if (!get_len_file(stream_output, &len2)) {
        close(stream_input);
        close(stream_output);
        return 1;
    };
    int dif = len2 - len1;

    if (!insert_new_phdr(stream_output, len1, dif)) {
        printf("Error: could not insert new program header\n");
        close(stream_input);
        close(stream_output);
        return 1;
    };

    if (!increment_program_header(stream_output)) {
        printf("Error: could not increment program header\n");
        close(stream_input);
        close(stream_output);
        return 1;
    };


    // cree le nouveau programme header, qui pointe sur le segement ajouter a la fin

    // faut store l'offset

    // inserer un nouveau programme header

    // Copier toutes les data d'apres pour les reecrirr apres


    // modifier le nomber de programme header


    // function qui compte le nombre d'octet du fichier -> pour remplacer la valeur dans le header

    //reecrire apres

    // function qui remplace la valeur dans le header
    //reecrire apres

    // modifier le entrypoint pour pointer sur le segment qu'on ajoute
    //reecrire apres


    close(stream_input);
    close(stream_output);
    return 0;
}
