#!/bin/bash
set -e

# Create ISO structure
mkdir -p isodir/boot/grub
cp kernel.elf isodir/boot/

# Create GRUB config
cat > isodir/boot/grub/grub.cfg << 'EOF'
set timeout=0
set default=0

menuentry "SO_Decentralizado" {
    multiboot2 /boot/kernel.elf
}
EOF

# Create ISO
grub-mkrescue -o kernel.iso isodir

echo "ISO created successfully: kernel.iso"
