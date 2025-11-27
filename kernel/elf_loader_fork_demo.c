/* kernel/elf_loader_fork_demo.c
 * Load and run the embedded fork demo user program
 */

#include "elf_loader.h"
#include "drivers/serial.h"
#include "user_fork_bin.h"

void elf_loader_fork_demo(void) {
    serial_puts("[elf_fork_demo] Loading embedded user fork ELF\n");
    process_t *proc = elf_load(user_fork_demo_elf, user_fork_demo_elf_len);
    if (!proc) {
        serial_puts("[elf_fork_demo] elf_load failed\n");
        return;
    }

    serial_puts("[elf_fork_demo] Executing user fork demo (will iret to ring-3)\n");
    elf_exec(proc, NULL, NULL);

    serial_puts("[elf_fork_demo] elf_exec returned (unexpected)\n");
}
