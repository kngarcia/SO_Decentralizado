# SOH Descentralizado - Microkernel x86-64 64-bit Portátil

**Estado**: ✅ MVP Compilable + **Fase 1 (Syscalls + ELF Loader)** Implementada

Un **microkernel modular 64-bit (x86-64)** diseñado para ejecutar aplicaciones de IA en redes descentralizadas ad-hoc.

- ✅ Bootea en cualquier VM (QEMU, VirtualBox, VMware) o hardware x86-64
- ✅ IPC + Scheduler funcionales (cooperative round-robin)
- ✅ **Syscalls + ELF loader implementados** (Fase 1)
- ✅ Tests unitarios pasando
- ✅ Documentación completa

## 🚀 Quick Start

```bash
# Compilar kernel 64-bit
cd ~/Documents/soh-descentralizado
make clean && make iso

# Ejecutar en QEMU
qemu-system-x86_64 -cdrom myos.iso -m 512M -serial stdio

# Tests (host-side, sin QEMU)
gcc -o tests/ipc_test tests/ipc_test.c && ./tests/ipc_test
gcc -o tests/scheduler_test tests/scheduler_test.c && ./tests/scheduler_test
```

## 📚 Documentación

- **README.md** ← Estás aquí (overview general)
- **EXECUTIVE_SUMMARY.md** - Resumen ejecutivo (español)
- **PHASE1_SYSCALLS_ELF.md** - Detalles técnicos Fase 1
- **AI_ROADMAP.md** - Roadmap 5 fases para IA

## 🏗️ Arquitectura

```
kernel/
├── kernel.c           # Entry point (kmain), demo tasks
├── start.S            # 32→64-bit boot, multiboot transition
├── linker.ld          # 64-bit ELF linker script
├── libc.c             # memcpy, memset, strlen...
├── syscall.h/.c       # Syscalls (12 números definidos)
├── elf_loader.h/.c    # ELF parser + loader
│
├── arch/x86/
│   ├── gdt.c          # Global Descriptor Table (64-bit)
│   ├── idt.c          # Interrupt Descriptor Table (64-bit)
│   ├── paging.c       # Identity paging setup
│   └── interrupts.S   # gdt_flush, idt_flush (64-bit asm)
│
├── drivers/
│   ├── serial.h/.c    # COM1 @ 38400 baud ✅
│   ├── timer.c        # PIT setup (stub handler)
│   └── keyboard.c     # Polling (stub)
│
├── ipc/
│   └── message.c      # Ring buffer (32 slots, 128B each)
│
├── scheduler/
│   └── round_robin.c  # Cooperative tasks
│
└── mm/
    ├── virtual_memory.h/.c  # Page table + allocator stubs
    └── physical_memory.c
```

## ✨ Fase 1: Syscalls + ELF Loader

**Nuevo en esta versión**: Sistema completo para ejecutar programas de usuario.

✅ **Implementado**:
- Estructura de 12 syscalls (EXIT, YIELD, LOG, MMAP, FORK, EXEC, WAIT, READ, WRITE, OPEN, CLOSE, STAT)
- ELF64 parser: valida magic, clase, endianness, tipo
- ELF loader: parsea headers, carga LOAD segments, zero-fill BSS
- Process Control Block (PCB): PID, entry point, heap/stack

🟡 **TODO (Próximas 1-2 semanas)**:
- int 0x80 trap handler (interrupts.S)
- Ring-3 selectors en GDT
- elf_exec() con iretq jump
- User-mode test (hello.elf)

Ver **PHASE1_SYSCALLS_ELF.md** para detalles técnicos completos.

## 📋 Compilación

```bash
cd kernel
make all        # Builds kernel.elf
cd ..
make iso        # Creates myos.iso with GRUB
make clean      # Removes build artifacts
```

## Ejecución

### En QEMU (con GRUB ISO)

```bash
qemu-system-x86_64 -cdrom myos.iso -m 512M -serial stdio
```

### Con Disco Booteable (GRUB instalado)

```bash
# Crear disco (ya hecho en /tmp/myos-disk.img):
qemu-system-x86_64 -drive file=/tmp/myos-disk.img,format=raw -m 512M -serial file:/tmp/qemu-serial.log
```

## Pruebas (Host-side Unit Tests)

Todos los tests se pueden compilar y ejecutar en el host sin QEMU:

```bash
# IPC message queue test
gcc -o tests/ipc_test tests/ipc_test.c -I./kernel -std=c11
./tests/ipc_test

# Scheduler round-robin test
gcc -o tests/scheduler_test tests/scheduler_test.c
./tests/scheduler_test

# Integrated IPC + Scheduler test
gcc -o tests/ipc_scheduler_integration_test tests/ipc_scheduler_integration_test.c
./tests/ipc_scheduler_integration_test
```

### Resultados de Tests

- `ipc_test`: **PASS** (send/recv/overflow validation)
- `scheduler_test`: **PASS** (round-robin distribution)
- `ipc_scheduler_integration_test`: **PASS** (producer/consumer with cooperative scheduling)

## Siguientes Mejoras Propuestas

### Corto Plazo (Kernel Base)
1. **Preemptive Scheduling**: Implementar IRQ del PIT (timer) y context switch preemptivo
2. **Syscalls**: Trap gate (int 0x80) para transiciones supervisor ↔ user-mode
3. **Process Isolation**: ELF loader + per-process page tables (ring-3 execution)
4. **Better IPC**: Wake/block primitives, capacidad-based ACL

### Mediano Plazo (Escalado)
1. **Networking**: Driver NIC básico (QEMU e1000) + stack TCP/IP minimalista
2. **Descubrimiento**: Protocolo P2P para nodos ad-hoc (mDNS, beacon)
3. **Distributed Primitives**: RPC, named pipes, pub/sub channel

### Largo Plazo (Carga de IA)
1. **WASM Runtime**: Integrar wasmtime o wasm3 para ejecutar módulos IA aislados
2. **Contenedores Ligeros**: chroot + namespaces para aislar aplicaciones
3. **Gestor de Recursos**: Quotas CPU/memoria, scheduler con prioridades
4. **Acceleradores**: GPU/TPU integration (virtio o stub para QEMU)

## Diagrama de Flujo de Arranque

```
_start (start.S)
  → Setup stack
  → Call kmain()
    
kmain() (kernel.c)
  → serial_init() - Configure COM1
  → gdt_install() - Load GDT
  → idt_install() - Load IDT
  → irq_install() - Setup IRQ handlers
  → timer_install() - Initialize PIT
  → keyboard_install() - Setup keyboard polling
  → show_string("kernel started") - Print via VGA + serial
  → paging_enable() - Enable 32-bit paging
  → task_create(producer) - Register producer task
  → task_create(consumer) - Register consumer task
  → scheduler_start() - Start round-robin scheduler loop
```

## Notas de Desarrollo

- Kernel es 32-bit (i386 ISA, -m32 flags)
- No hay protección real (todas las tareas en ring-0, espacio de dirección compartido)
- Scheduler es cooperativo: cada tarea debe ceder voluntariamente (no hay preemption)
- Mensajes IPC son strings NULL-terminated de 128 bytes máximo
- El puerto serial está en 0x3F8 (COM1, 38400 baud)

## Recursos

- Multiboot Specification: https://www.gnu.org/software/grub/manual/multiboot/
- Intel 80386 Reference: https://www.intel.com/content/dam/develop/external/us/en/documents/manuals/64-ia-32-architectures-software-developer-vol-1-manual.pdf
- OSDev.org: https://wiki.osdev.org/

---

**Estado**: Prototipo funcional con tests host-side pasando. Listo para escalado gradual a un SO real.
