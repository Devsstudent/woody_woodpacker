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
        if ((stream_output = open("./woody", O_CREAT | O_WRONLY, 0755)) == -1)
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
    close(stream_input);
    write_woody(stream_output);

    // function qui compte le nombre d'octet du fichier -> pour remplacer la valeur dans le header
    // function qui remplace la valeur dans le header
    // modifier le entrypoint pour pointer sur le segment qu'on ajoute
    // ajouter le segment a la fin
    // inserer un nouveau programme header
    // modifier le nomber de programme header
    close(stream_output);
    return 0;
}