/* kernel/elf_loader.h
 * ELF binary loader - Parses and loads user-mode ELF binaries
 * Supports x86-64 little-endian ELF files (ET_EXEC, ET_DYN)
 */

#ifndef ELF_LOADER_H
#define ELF_LOADER_H

#include <stdint.h>
#include <stddef.h>

/* ELF Header constants */
#define EI_MAG0        0
#define EI_MAG1        1
#define EI_MAG2        2
#define EI_MAG3        3
#define EI_CLASS       4
#define EI_DATA        5
#define EI_VERSION     6
#define EI_OSABI       7

#define ELFMAG0        0x7f
#define ELFMAG1        'E'
#define ELFMAG2        'L'
#define ELFMAG3        'F'

#define ELFCLASS64     2
#define ELFDATA2LSB    1  /* Little-endian */

#define ET_EXEC        2
#define ET_DYN         3

#define PT_LOAD        1
#define PT_DYNAMIC     3

/* ELF64 structures */
typedef struct {
    uint8_t  e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} elf64_hdr_t;

typedef struct {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} elf64_phdr_t;

/* Process control block (simplified) */
typedef struct {
    uint64_t pid;
    uint64_t entry_point;      /* Entry point (e_entry) */
    uint64_t heap_start;
    uint64_t heap_end;
    uint64_t stack_top;
    uint64_t *page_table;      /* Per-process page table */
    int      state;            /* 0=new, 1=running, 2=sleeping, 3=dead */
} process_t;

/* Validate ELF header */
int elf_validate(const elf64_hdr_t *hdr);

/* Load ELF binary into memory
 * Returns pointer to process_t if successful, NULL otherwise
 */
process_t *elf_load(const uint8_t *binary_data, size_t size);

/* Execute process (jump to entry point in ring-3)
 * Sets up initial stack frame and registers
 */
int elf_exec(process_t *proc, char **argv, char **envp);

/* Free process resources */
void elf_free_process(process_t *proc);

#endif /* ELF_LOADER_H */
