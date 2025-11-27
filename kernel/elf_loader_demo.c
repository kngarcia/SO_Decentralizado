/* kernel/elf_loader_demo.c
 * Simple demo that loads the embedded user ELF (user_hello) and executes it.
 */

#include "elf_loader.h"
#include "drivers/serial.h"
#include "user_hello_bin_nocet.h"

void elf_loader_demo(void) {
    serial_puts("[elf_demo] Loading embedded user ELF (no-CET)\n");
    process_t *proc = elf_load(build_user_hello_nocet_elf, build_user_hello_nocet_elf_len);
    if (!proc) {
        serial_puts("[elf_demo] elf_load failed\n");
        return;
    }

    serial_puts("[elf_demo] Executing user process (will iret to ring-3)\n");
    /* Debug: dump first bytes at entry to ensure user code memory looks sane */
    /* DISABLED: this loop seems to hang in some builds
    unsigned char *entry_bytes = (unsigned char *)proc->entry_point;
    serial_puts("[elf_demo] entry bytes: ");
    for (int i = 0; i < 32; ++i) {
        serial_put_hex(entry_bytes[i]); serial_putc(' ');
    }
    serial_putc('\n');
    */

    serial_puts("[elf_demo] about to call elf_exec\n");
    elf_exec(proc, NULL, NULL);

    /* If we get here, the user process returned unexpectedly */
    serial_puts("[elf_demo] elf_exec returned (unexpected)\n");
}
