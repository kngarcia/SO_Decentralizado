/* kernel/elf_loader_demo.c
 * Simple demo that loads the embedded user ELF (user_hello) and executes it.
 */

#include "elf_loader.h"
#include "drivers/serial.h"
#include "user_hello_bin.h"

void elf_loader_demo(void) {
    serial_puts("[elf_demo] Loading embedded user ELF\n");
    process_t *proc = elf_load(build_user_hello_elf, build_user_hello_elf_len);
    if (!proc) {
        serial_puts("[elf_demo] elf_load failed\n");
        return;
    }

    serial_puts("[elf_demo] Executing user process (will iret to ring-3)\n");
    elf_exec(proc, NULL, NULL);

    /* If we get here, the user process returned unexpectedly */
    serial_puts("[elf_demo] elf_exec returned (unexpected)\n");
}
