# Estrategia para Ejecutar Cargas de IA en SOH Descentralizado

Este documento esboza un enfoque progresivo para integrar y ejecutar aplicaciones de inteligencia artificial en el microkernel SOH.

## Fases de Implementación

### Fase 1: IA en Userland (Semanas 1-2)

**Objetivo**: Ejecutar modelos pequeños (tinyML) en procesos user-mode aislados.

#### 1.1 Implementar Syscalls Básicas
- `sys_exit()` - Terminar proceso
- `sys_yield()` - Ceder CPU (voluntario)
- `sys_log()` - Imprimir a serial
- `sys_allocate()` - Pedir memoria al kernel

```c
// syscall stub en kernel/arch/x86/syscalls.c
void handle_syscall(int syscall_id, ...) {
    switch (syscall_id) {
        case SYS_EXIT:    process_exit(...); break;
        case SYS_YIELD:   scheduler_yield(); break;
        case SYS_LOG:     serial_puts(...); break;
        case SYS_ALLOC:   alloc_from_heap(...); break;
    }
}
```

#### 1.2 ELF Loader Minimalista
- Parsear header ELF (magic, program headers)
- Mapear segmentos en memoria user-mode (ring-3)
- Configurar página de heap/stack por proceso
- Saltar a entry point

```c
// kernel/loader/elf.c
int elf_load_and_exec(const char *elf_data, size_t size) {
    elf_header_t *hdr = (elf_header_t *)elf_data;
    // Validate magic
    // Map segments to per-process pagetable
    // Transition to ring-3 and jump to entry
}
```

#### 1.3 Test: Pequeño Programa IA (C)
```c
// user/ai_demo.c - Simple linear regression predictor
#include "user_syscalls.h"

int main() {
    // Tiny ML: load pre-trained weights, run inference
    float weights[10] = {0.1, 0.2, ...};
    float input[10] = {...};
    float output = 0.0;
    for (int i = 0; i < 10; i++) output += weights[i] * input[i];
    
    sys_log("AI output: ");
    sys_log_float(output);
    sys_exit(0);
}
```

Compilar:
```bash
gcc -m32 -nostdlib user/ai_demo.c -o ai_demo.elf
```

### Fase 2: WASM Runtime (Semanas 3-4)

**Objetivo**: Ejecutar aplicaciones en WebAssembly para portabilidad y seguridad.

#### 2.1 Integrar WASM Runtime Minimalista
Opciones:
- **wasm3** (tiny, ~50 KB): https://github.com/wasm3/wasm3
- **wasmtime** (moderno, más grande): https://docs.wasmtime.dev/
- **Custom VM** (minimal): interpreter simple de bytecode WASM

Recomendación: **wasm3** para kernel embebido.

#### 2.2 WASM Loader en Kernel

```c
// kernel/wasm/wasm_loader.c
typedef struct {
    wasm3_env_t *env;
    wasm3_runtime_t *rt;
    wasm3_module_t *module;
    wasm3_function_t *main_fn;
} wasm_instance_t;

int wasm_load(const uint8_t *wasm_binary, size_t size, wasm_instance_t *out) {
    out->env = m3_NewEnvironment();
    out->rt = m3_NewRuntime(out->env, 64*1024, NULL);
    m3_LoadModule(out->rt, &out->module, wasm_binary);
    m3_FindFunction(&out->main_fn, out->rt, "main");
    return 0;
}

int wasm_exec(wasm_instance_t *inst) {
    M3Result result = m3_Call(inst->main_fn);
    return (result == m3_succeed) ? 0 : -1;
}
```

#### 2.3 Test: IA en WASM
```rust
// user/ai_wasm.rs - WASM module with simple CNN
#[no_mangle]
pub extern "C" fn main() -> i32 {
    let weights = [0.1, 0.2, 0.3, ...]; // Pre-trained tiny model
    let input = [1.0, 2.0, ...];
    let mut output = 0.0;
    for i in 0..weights.len() {
        output += weights[i] * input[i];
    }
    output as i32
}
```

Compilar:
```bash
rustc --target wasm32-unknown-unknown user/ai_wasm.rs -o ai_wasm.wasm
```

### Fase 3: Carga de Modelos Distribuida (Semanas 5-6)

**Objetivo**: Transferir y ejecutar modelos entre nodos ad-hoc.

#### 3.1 Protocolo de Transferencia de Modelos

Usar mDNS + UDP multicast para descubrimiento:

```c
// Protocol: ANNOUNCE nodo -> broadcast
struct announce_pkt {
    uint16_t node_id;
    char name[32];
    uint32_t model_hash;
    uint16_t model_size;
};

// Protocol: REQUEST nodo1 -> nodo2
struct request_pkt {
    uint32_t model_hash;
    uint32_t chunk_offset;
};

// Protocol: DATA nodo2 -> nodo1 (UDP)
struct data_pkt {
    uint32_t model_hash;
    uint32_t offset;
    uint16_t chunk_len;
    uint8_t data[512];
};
```

#### 3.2 Gestor de Modelos en Kernel

```c
// kernel/ai/model_cache.c
#define MODEL_CACHE_SIZE (16*1024*1024)  // 16 MB

typedef struct {
    uint32_t hash;
    uint32_t size;
    uint8_t *data;
    timestamp_t loaded_at;
    int usage_count;
} cached_model_t;

int model_cache_store(uint32_t hash, const uint8_t *data, uint32_t size) {
    // LRU eviction if cache full
    // Store in memory or offload to disk if available
}

int model_cache_get(uint32_t hash, uint8_t **out) {
    // Retrieve from cache or request from peer
}
```

#### 3.3 Test: Transferencia entre Nodos

```bash
# Nodo 1: anunciar modelo
qemu-system-x86_64 -netdev tap,id=net0 ... &

# Nodo 2: conectarse a red, descubrir, descargar, ejecutar
qemu-system-x86_64 -netdev tap,id=net0 ... &

# Resultado: Nodo 2 ejecuta modelo del Nodo 1
```

### Fase 4: Aceleración y Optimización (Semanas 7-8)

**Objetivo**: Mejorar rendimiento para cargas reales.

#### 4.1 Preemptive Scheduler con Prioridades

```c
// kernel/scheduler/preemptive.c
typedef struct {
    int pid;
    task_fn entry;
    uint32_t priority;       // 0-10 (higher = more frequent)
    uint64_t time_quantum;   // ms per slice
    uint64_t total_cpu_us;   // total CPU time
    task_state_t state;      // READY, RUNNING, BLOCKED, ZOMBIE
} process_t;

void schedule_next() {
    // PIT IRQ triggers this
    // Save current process state (regs, stack)
    // Pick next highest-priority READY task
    // Restore and jump to next task
}
```

#### 4.2 Memory Gestor Mejorado

```c
// kernel/mm/heap.c - Buddy allocator for fast alloc/free
typedef struct {
    uint8_t *base;
    size_t size;
    bitmap_t free_blocks;  // Track free/used pairs
} buddy_heap_t;

void *malloc(size_t size) {
    // Find smallest power-of-2 block >= size
    // Mark as allocated, return pointer
}

void free(void *ptr) {
    // Mark as free, coalesce adjacent free blocks
}
```

#### 4.3 Caché Inteligente de Modelos

```c
// Cargar frecuentemente en RAM, offload de baja freq a net
// Precalentar modelos usados frecuentemente
// Comprensión de modelos (cuantización, pruning)
```

### Fase 5: GPU/Accelerator Support (Semanas 9-10) [Opcional]

#### 5.1 virtio-gpu / QEMU GPU

```c
// kernel/drivers/virtio_gpu.c
// Delegate compute-heavy ops to host GPU via virtio
```

#### 5.2 Offload a Host (fallback)

```c
// RPC a proceso en el host que corre TensorFlow/ONNX
// Kernel actúa como cliente, host es servidor compute
```

---

## Stack Propuesto

### Base (Ya Implementado)

- Kernel: 32-bit x86, multiboot GRUB
- Scheduler: Round-robin cooperativo
- IPC: Ring buffer simple
- Serial: Logging básico

### Fase 1 Add-ons

- Syscalls: exit, yield, log, allocate
- ELF Loader: parseo mínimo + ring-3 jump
- User Heap: malloc/free simple

### Fase 2 Add-ons

- WASM3 runtime (50-100 KB)
- WASM loader + executor

### Fase 3 Add-ons

- NIC driver (e1000 emulado en QEMU)
- UDP stack minimalista
- mDNS beacon + discovery
- Model cache (filesystem fallback)

### Fase 4 Add-ons

- Preemptive scheduler con timer IRQ
- Buddy allocator
- Memory quotas per-process

### Fase 5 Add-ons [Opcional]

- virtio driver
- GPU-side compute delegation

---

## Ejemplo: End-to-End (Fase 3)

```bash
# Compilar microkernel con fase 1-2 support
make clean && make iso

# Boot nodo 1 (provee modelo)
qemu-system-x86_64 -cdrom myos.iso \
  -netdev user,id=net0,hostfwd=udp::5353-:5353 \
  -device e1000,netdev=net0 \
  -m 512M -serial stdio

# (en otra terminal) Boot nodo 2 (consume modelo)
qemu-system-x86_64 -cdrom myos.iso \
  -netdev user,id=net0,hostfwd=udp::5354-:5353 \
  -device e1000,netdev=net0 \
  -m 512M -serial stdio

# Output esperado:
# [Node1] MODEL_ANNOUNCE: tiny_cnn_v1 (hash=0xABCD, size=128KB)
# [Node2] MODEL_DISCOVERED: tiny_cnn_v1, downloading...
# [Node2] MODEL_READY, executing AI_WASM...
# [Node2] INFERENCE_RESULT: 0.87 (confidence)
```

---

## Benchmarks Objetivo

### Pequeño Modelo (128 KB, 1000 ops)

| Fase | Tiempo (ms) | Notas |
|------|-------------|-------|
| 1 (ELF nativo) | 5-10 | Sin optimización |
| 2 (WASM) | 10-20 | Interpretado |
| 3 (Cached) | 2-5 | Modelo en RAM |
| 4 (Preemptive) | 1-3 | Mejor isolation |

### Transferencia de Red (1 MB modelo)

| Fase | Tiempo (ms) | Notas |
|------|-------------|-------|
| 3 (UDP simple) | 100-200 | No fiable |
| 3+ (UDP con retry) | 150-300 | Tolerante |
| 4+ (TCP opt) | 50-100 | Posible upgrade |

---

## Referencias y Librerías

- **wasm3**: https://github.com/wasm3/wasm3 (embednable)
- **ONNX Runtime**: https://onnx.ai/onnx/operators/ (formatos estándar)
- **TensorFlow Lite**: https://www.tensorflow.org/lite (micro)
- **libVNCServer**: para visualización remota (opcional)
- **lwIP**: stack TCP/IP embebido (alternativa)

---

**Conclusión**: Este plan permite evolucionar desde un microkernel simple a un SO distribuido capaz de ejecutar cargas de IA reales en redes ad-hoc, manteniendo control sobre latencias, memoria y seguridad en cada fase.
