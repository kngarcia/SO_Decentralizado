# SOH Descentralizado - Fase 1: Syscalls + ELF Loader (x86-64, 64-bit)

## Actualización: Migración a 64-bit (x86-64)

El kernel ahora es **100% 64-bit** (x86-64), lo que significa:

✅ **Portátil**: Bootea en cualquier VM (QEMU, VirtualBox, VMware, Hyper-V) o hardware físico  
✅ **Moderno**: Aprovecha arquitectura x86-64 completa (16 registros de propósito general, modo long)  
✅ **Escalable**: Preparado para memoria >4GB, kernels más complejos  
✅ **UEFI/BIOS compatible**: Multiboot2 corre en cualquier bootloader (GRUB en VMs, firmware en metal)  

### Cambios Técnicos de 32-bit → 64-bit

| Componente | 32-bit | 64-bit |
|-----------|--------|--------|
| **Compilación** | `-m32` | `-m64` |
| **Linker** | `elf_i386` | `elf_x86_64` |
| **Stack inicial** | 0x4000 bytes | 0x8000 bytes |
| **Paging** | 2-level (PD, PT) | 4-level (PML4, PDPT, PD, PT) |
| **Registros base** | 32-bit (%eax, %ebx) | 64-bit (%rax, %rbx) |
| **Convención llamadas** | cdecl | System V AMD64 (RDI, RSI, RDX, RCX...) |

### Nuevo Boot Sequence (start.S)

```
GRUB (32-bit) → _start32 (32-bit asm)
  ↓ [setup paging: PML4/PDPT/PD]
  ↓ [enable PAE + long mode MSR]
  ↓ [ljmpl to long mode]
_start64 (64-bit code)
  ↓ [zero BSS]
  ↓ [call kmain(mbi_ptr)]
```

---

## Fase 1: Syscalls + ELF Loader

### Objetivo
Permitir que programas de usuario (ring-3) ejecuten en el kernel, con:
- Interface syscall (int 0x80, syscall instruction)
- ELF loader para binarios de usuario
- Ejemplo: pequeño programa que corre en ring-3 y hace sys_log

### Archivos Nuevos (Creados)

#### `kernel/syscall.h` y `kernel/syscall.c`
**Qué hace**: Define 12 syscalls básicas (SYS_EXIT, SYS_YIELD, SYS_LOG, SYS_MMAP, etc.)

```c
/* Syscall numbers (cross-arch compatible) */
#define SYS_EXIT       1   /* Termina proceso */
#define SYS_YIELD      2   /* Cede CPU */
#define SYS_LOG        3   /* Imprime a serial */
#define SYS_MMAP       4   /* Mapea memoria */
#define SYS_FORK       5   /* Clona proceso (TODO) */
#define SYS_EXEC       6   /* Ejecuta ELF (TODO) */
...

/* Dispatcher */
syscall_result_t syscall_dispatch(uint64_t num, arg1, arg2, arg3);
```

**Estado actual**: Stubs funcionales (todos compilables, algunos solo con logging)

#### `kernel/elf_loader.h` y `kernel/elf_loader.c`
**Qué hace**: Parsea y carga binarios ELF64 a memoria

```c
/* Valida cabecera ELF */
int elf_validate(const elf64_hdr_t *hdr);

/* Carga segmentos LOAD a memoria */
process_t *elf_load(const uint8_t *binary_data, size_t size);

/* Salta a entry point en ring-3 (TODO: implementar) */
int elf_exec(process_t *proc, char **argv, char **envp);
```

**Características**:
- ✅ Valida magic (0x7f 'E' 'L' 'F')
- ✅ Verifica 64-bit, little-endian, ET_EXEC/ET_DYN
- ✅ Parsea program headers (LOAD segments)
- ✅ Copia segmentos a memoria con memcpy
- ✅ Zero-fill BSS
- ✅ Crea PCB (process control block)
- ⚠️ TODO: Saltar a ring-3 (requiere TSS, selectors de ring-3, iret)

### Nuevo Código en kernel.c

```c
/* Inicializar syscalls en kmain */
syscall_install();
show_string("[kmain] Syscall interface installed\n");
```

### Compilación (64-bit)

```bash
cd ~/Documents/soh-descentralizado
make clean && make iso
# Resultado: kernel.elf (ELF64) + myos.iso (booteable GRUB)
```

**Flags de compilación ahora**:
```makefile
CFLAGS := -m64 -ffreestanding -O2 -Wall -Wextra -fno-asynchronous-unwind-tables
ASFLAGS := -m64
LD := ld -m elf_x86_64
```

---

## Roadmap: Próximos Pasos (Fase 1 Completa)

### 1️⃣ Trap Handler para int 0x80 (1 día)

Crear interrupt handler que:
1. Capture int 0x80 desde ring-3
2. Lea syscall número y args (RDI, RSI, RDX, RCX)
3. Llame a `syscall_dispatch()`
4. Retorne resultado en RAX

**Archivo**: Agregar a `kernel/arch/x86/interrupts.S`

```asm
.global isr_0x80
isr_0x80:
    push %rax
    mov %rdi, %rax          /* syscall num */
    mov %rsi, %rdi          /* arg1 */
    mov %rdx, %rsi          /* arg2 */
    mov %rcx, %rdx          /* arg3 */
    call syscall_dispatch
    pop %rbx
    iretq
```

### 2️⃣ Saltar a Ring-3 (1-2 días)

Implementar `elf_exec()` que:
1. Aloque page table per-proceso
2. Copie kernel mappings (kernel code/data)
3. Setup stack inicial (argc, argv, envp)
4. Cargue TSS con ring-3 stack
5. Use `iretq` para saltar a ring-3

**Pseudocódigo**:
```c
int elf_exec(process_t *proc, char **argv, char **envp) {
    /* 1. Crear per-process page table */
    uint64_t *new_pml4 = virtual_memory_alloc(0, 0x1000, PROT_WRITE);
    
    /* 2. Kernel mappings (copy del kernel PML4) */
    for (int i = 256; i < 512; i++) {
        new_pml4[i] = kernel_pml4[i];
    }
    
    /* 3. Stack inicial (user space) */
    void *user_stack = virtual_memory_alloc(0, 0x100000, PROT_READ|PROT_WRITE);
    
    /* 4. Setup registers y saltar */
    struct {
        uint64_t rip;
        uint64_t rsp;
        uint64_t cs;   /* selector ring-3 code (0x1B) */
        uint64_t ss;   /* selector ring-3 data (0x23) */
    } iret_frame;
    
    iret_frame.rip = proc->entry_point;
    iret_frame.rsp = (uint64_t)user_stack + 0x100000;
    iret_frame.cs = 0x1B;
    iret_frame.ss = 0x23;
    
    /* Saltar con iretq */
    asm("mov %0, %%rsp; iretq" : : "r"(&iret_frame));
}
```

### 3️⃣ Test: User-mode "Hello World" (2 días)

Crear ejemplo mínimo:

**`user/hello.c`**:
```c
#include <stdint.h>

/* Syscall inline asm */
static inline int64_t syscall(uint64_t num, uint64_t a1, uint64_t a2, uint64_t a3) {
    int64_t ret;
    asm volatile("int $0x80" 
        : "=a"(ret)
        : "D"(num), "S"(a1), "d"(a2), "c"(a3)
    );
    return ret;
}

void _start(void) {
    syscall(3, (uint64_t)"[user] Hello from ring-3!\n", 0, 0);  /* SYS_LOG */
    syscall(1, 0, 0, 0);  /* SYS_EXIT */
}

/* Minimal ELF sections */
.section .text
.globl _start
```

**Build**:
```bash
gcc -m64 -ffreestanding -nostartfiles -c user/hello.c -o user/hello.o
ld -m elf_x86_64 -Ttext 0x400000 user/hello.o -o user/hello.elf
```

**Test en kernel**:
```c
/* kernel.c kmain() */
extern process_t *elf_load(...);
extern int elf_exec(...);

process_t *proc = elf_load(hello_elf_data, hello_elf_size);
if (proc) {
    elf_exec(proc, NULL, NULL);
    /* Aquí salta a ring-3... */
}
```

---

## Estado Actual de Subsistemas (Post-Fase 1 Setup)

| Subsistema | Estado | Prioridad Fase 1 |
|-----------|--------|-----------------|
| **Syscalls** | Headers + stubs | 🔴 **BLOQUEADOR** |
| **ELF Loader** | Validación + segmentos | 🔴 **BLOQUEADOR** |
| **Ring-3 Jump** | Stubs (TODO) | 🔴 **BLOQUEADOR** |
| **int 0x80** | IDT instalado, handler falta | 🟡 **CRÍTICO** |
| **GDT/Ring-3** | GDT exists, ring-3 selectors TODO | 🟡 **CRÍTICO** |
| **Paging** | Identity map 2GB, per-process TODO | 🟡 **CRÍTICO** |
| **IPC** | Funcional (ring-0 only) | 🟢 Listo |
| **Scheduler** | Funcional (cooperative) | 🟢 Listo |
| **Serial** | Funcional | 🟢 Listo |

---

## Compilación y Ejecución

### Build 64-bit (Portátil)

```bash
cd ~/Documents/soh-descentralizado
make clean && make iso

# Resultado:
# - kernel/kernel.elf (ELF64, entrelazable)
# - kernel.elf (en root, copiado para boot)
# - myos.iso (imagen GRUB booteable)
```

### QEMU (Cualquier arquitectura, incluido ARM Mac con Rosetta)

```bash
# VM x86-64
qemu-system-x86_64 -cdrom myos.iso -m 512M -serial stdio

# Output esperado:
# === SOH Descentralizado (64-bit x86-64 Kernel) ===
# myos: kernel started (portable x86-64 image)
# [kmain] Syscall interface installed
# [kmain] Starting cooperative round-robin scheduler...
# ...
```

### VirtualBox / VMware / Hyper-V

Misma ISO (`myos.iso`), funciona idénticamente.

---

## Migración desde Entorno 32-bit (Si necesario revertir)

No es necesario. El código 64-bit es **superconjunto** funcional:
- Todos los drivers (serial, timer, keyboard) funcionan igual
- Convencion de llamadas es compatible
- Memory layout es más simple (identity map 2GB es suficiente)

Si necesitas 32-bit puro: checkout a versión anterior o usa `-m32` en Makefile.

---

## Próximas Tareas (TODO List)

### Fase 1 Checkpoint (Hoy/Mañana)

- [ ] Implementar int 0x80 handler en interrupts.S
- [ ] Agregar GDT ring-3 selectors (0x1B code, 0x23 data)
- [ ] Implementar elf_exec() con iretq jump
- [ ] Crear user/hello.c test
- [ ] Build + test en QEMU

### Fase 2 (Post-Phase 1)

- [ ] WASM3 runtime (embedding en kernel)
- [ ] Fork/exec syscalls completos
- [ ] Preemptive scheduler (timer IRQ handler)

---

**Versión**: 0.1.0-phase1  
**Arquitectura**: x86-64 (64-bit)  
**Compilación**: GCC -m64, LD elf_x86_64  
**Boot**: Multiboot2 GRUB (portátil)
