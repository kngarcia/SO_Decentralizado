/* kernel/drivers/serial.h
 * Serial port (COM1) interface for kernel logging
 */

#ifndef SERIAL_H
#define SERIAL_H

void serial_init(void);
void serial_putc(char c);
void serial_puts(const char *s);
void serial_put_hex(uint64_t value);

#endif /* SERIAL_H */
