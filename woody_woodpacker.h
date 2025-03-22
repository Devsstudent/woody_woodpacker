#ifndef WOODY_WOODPACKER_H
#define WOODY_WOODPACKER_H

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>

typedef uint64_t	Elf64_Addr;
typedef uint16_t	Elf64_Half;
typedef uint64_t	Elf64_Off;
typedef int32_t		Elf64_Sword;
typedef int64_t		Elf64_Sxword;
typedef uint32_t	Elf64_Word;
typedef uint64_t	Elf64_Lword;
typedef uint64_t	Elf64_Xword;

typedef struct elf64_phdr {
    Elf64_Word p_type;
    Elf64_Word p_flags;
    Elf64_Off p_offset;		/* Segment file offset */
    Elf64_Addr p_vaddr;		/* Segment virtual address */
    Elf64_Addr p_paddr;		/* Segment physical address */
    Elf64_Xword p_filesz;		/* Segment size in file */
    Elf64_Xword p_memsz;		/* Segment size in memory */
    Elf64_Xword p_align;		/* Segment alignment, file & memory */
  } Elf64_Phdr;

/*
typedef uint64_t	Elf64_Addr;
typedef uint16_t	Elf64_Half;
typedef uint64_t	Elf64_Off;
typedef int32_t		Elf64_Sword;
typedef int64_t		Elf64_Sxword;
typedef uint32_t	Elf64_Word;
typedef uint64_t	Elf64_Lword;
typedef uint64_t	Elf64_Xword;
*/

/*
typedef struct {
        unsigned char   e_ident[EI_NIDENT]; 16 / 0
        Elf64_Half      e_type; 2 / 16
        Elf64_Half      e_machine; 2 / 18
        Elf64_Word      e_version; 4 / 20
        Elf64_Addr      e_entry; 8 / 24
        Elf64_Off       e_phoff; 8 / 32 
        Elf64_Off       e_shoff; 8 / 40
        Elf64_Word      e_flags; 4 / 48
        Elf64_Half      e_ehsize; 2 / 52
        Elf64_Half      e_phentsize;  2 / 54
        Elf64_Half      e_phnum; 2 / 56
        Elf64_Half      e_shentsize; 2 / 58
        Elf64_Half      e_shnum; 2 / 60
        Elf64_Half      e_shstrndx; 2 / 62
} Elf64_Ehdr; / 64
*/
#endif