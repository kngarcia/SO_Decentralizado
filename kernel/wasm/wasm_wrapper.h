#ifndef WASM_WRAPPER_H
#define WASM_WRAPPER_H

#include <stdint.h>
#include <stddef.h>

// WASM3 module handle
typedef int wasm_module_id_t;

// Initialize WASM3 runtime
void wasm_init(void);

// Load a WASM module from memory
wasm_module_id_t wasm_load_module(const uint8_t *wasm_bytes, size_t len, const char *module_name);

// Execute a function in a loaded WASM module
int wasm_exec_function(wasm_module_id_t module_id, const char *func_name, int argc, const char **argv);

// Unload a WASM module
void wasm_unload_module(wasm_module_id_t module_id);

// Get last error message
const char* wasm_get_last_error(void);

#endif // WASM_WRAPPER_H
