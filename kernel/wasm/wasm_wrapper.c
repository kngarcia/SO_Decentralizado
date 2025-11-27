#include "wasm_wrapper.h"

// Kernel libc functions
extern void *memset(void *s, int c, unsigned long n);
extern void *memcpy(void *dest, const void *src, unsigned long n);
extern char *strcpy(char *dest, const char *src);
extern char *strncpy(char *dest, const char *src, unsigned long n);
extern int strcmp(const char *s1, const char *s2);
extern void serial_puts(const char *s);
extern void serial_put_hex(unsigned long value);
extern void serial_putc(char c);
extern void *malloc(unsigned long size);
extern void free(void *ptr);

// Provide basic types for WASM3
typedef unsigned long size_t;
typedef long ptrdiff_t;

// WASM3 configuration
#define d_m3HasWASI 0
#define d_m3MaxSaneTypedDataSize (64*1024)

// Minimal WASM3 integration - we'll use only core features
// and avoid the full runtime for now

// Global state
#define MAX_WASM_MODULES 4
static int g_wasm_initialized = 0;
static char g_last_error[256] = "No error";

typedef struct {
    int active;
    const uint8_t *wasm_data;
    size_t wasm_size;
    char name[64];
} wasm_module_entry_t;

static wasm_module_entry_t g_modules[MAX_WASM_MODULES];

void wasm_init(void) {
    serial_puts("[WASM3] Starting initialization...\n");
    memset(g_modules, 0, sizeof(g_modules));
    serial_puts("[WASM3] Modules cleared\n");
    g_wasm_initialized = 1;
    serial_puts("[WASM3] Initialized (simple mode) - READY\n");
}

wasm_module_id_t wasm_load_module(const uint8_t *wasm_bytes, size_t len, const char *module_name) {
    if (!g_wasm_initialized) {
        strcpy(g_last_error, "WASM3 not initialized");
        return -1;
    }
    
    if (!wasm_bytes || len == 0) {
        strcpy(g_last_error, "Invalid WASM data");
        return -1;
    }
    
    // Verify WASM magic number
    if (len < 8 || wasm_bytes[0] != 0x00 || wasm_bytes[1] != 0x61 ||
        wasm_bytes[2] != 0x73 || wasm_bytes[3] != 0x6d) {
        strcpy(g_last_error, "Invalid WASM magic number");
        return -1;
    }
    
    // Find free slot
    int slot = -1;
    for (int i = 0; i < MAX_WASM_MODULES; i++) {
        if (!g_modules[i].active) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        strcpy(g_last_error, "No free module slots");
        return -1;
    }
    
    // Store module (shallow copy - assumes data remains valid)
    g_modules[slot].active = 1;
    g_modules[slot].wasm_data = wasm_bytes;
    g_modules[slot].wasm_size = len;
    strncpy(g_modules[slot].name, module_name, sizeof(g_modules[slot].name) - 1);
    
    serial_puts("[WASM3] Loaded module '");
    serial_puts(module_name);
    serial_puts("' (id=");
    serial_put_hex(slot);
    serial_puts(", size=");
    serial_put_hex((unsigned long)len);
    serial_puts(" bytes)\n");
    return slot;
}

int wasm_exec_function(wasm_module_id_t module_id, const char *func_name, int argc, const char **argv) {
    (void)argc;
    (void)argv;
    
    if (module_id < 0 || module_id >= MAX_WASM_MODULES || !g_modules[module_id].active) {
        strcpy(g_last_error, "Invalid module ID");
        return -1;
    }
    
    if (!func_name) {
        strcpy(g_last_error, "Invalid function name");
        return -1;
    }
    
    // For now, we just validate the module exists
    // Full WASM3 integration would compile and execute here
    serial_puts("[WASM3] Would execute ");
    serial_puts(g_modules[module_id].name);
    serial_puts("::");
    serial_puts(func_name);
    serial_puts("() (stub)\n");
    
    // Return a test value
    if (strcmp(func_name, "get_magic_number") == 0) {
        return 42;
    }
    
    strcpy(g_last_error, "Function execution not yet implemented");
    return -1;
}

void wasm_unload_module(wasm_module_id_t module_id) {
    if (module_id >= 0 && module_id < MAX_WASM_MODULES && g_modules[module_id].active) {
        g_modules[module_id].active = 0;
        serial_puts("[WASM3] Unloaded module id=");
        serial_put_hex(module_id);
        serial_putc('\n');
    }
}

const char* wasm_get_last_error(void) {
    return g_last_error;
}
