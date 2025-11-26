# top-level Makefile
.PHONY: all iso run clean

all:
	@echo "Building kernel..."
	$(MAKE) -C kernel all

iso: all
	@echo "Creating ISO with GRUB..."
	rm -rf isodir
	mkdir -p isodir/boot/grub
	# copy kernel.elf to ISO
	cp kernel.elf isodir/boot/kernel.elf
	# grub.cfg (single-shell printf to avoid heredoc/shell-invocation issues)
	printf '%s\n' 'set timeout=5' 'set default=0' '' 'menuentry "myos" {' '  multiboot2 /boot/kernel.elf' '  boot' '}' > isodir/boot/grub/grub.cfg
	grub-mkrescue -o myos.iso isodir 2>/dev/null || xorriso -as mkisofs -R -J -o myos.iso isodir

run: iso
	qemu-system-x86_64 -cdrom myos.iso -m 256M -boot d

clean:
	$(MAKE) -C kernel clean
	rm -rf isodir myos.iso
