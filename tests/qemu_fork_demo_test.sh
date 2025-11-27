#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

echo "Building ISO with RUN_FORK_DEMO..."
make -C kernel CFLAGS='-DRUN_FORK_DEMO' all >/dev/null

rm -rf isodir
mkdir -p isodir/boot/grub
cp kernel.elf isodir/boot/kernel.elf
cat > isodir/boot/grub/grub.cfg <<'GRUB'
set timeout=5
set default=0

menuentry "myos" {
  multiboot2 /boot/kernel.elf
  boot
}
GRUB

grub-mkrescue -o myos.iso isodir 2>/dev/null || xorriso -as mkisofs -R -J -o myos.iso isodir

mkdir -p tmp
SERIAL_LOG="tmp/qemu_fork_serial.log"
rm -f "$SERIAL_LOG"

if ! command -v qemu-system-x86_64 >/dev/null 2>&1; then
  echo "qemu-system-x86_64 not found in PATH — please install QEMU to run this test."
  exit 2
fi

echo "Running QEMU for 10s (capturing serial to $SERIAL_LOG)"
timeout 10s qemu-system-x86_64 -cdrom myos.iso -m 512M -serial file:$SERIAL_LOG >/dev/null 2>&1 || true

echo "Checking serial output for both parent & child messages..."
FOUND_PARENT=$(grep -c "fork parent got pid" "$SERIAL_LOG" || true)
FOUND_CHILD=$(grep -c "fork child says hello" "$SERIAL_LOG" || true)

if [ "$FOUND_PARENT" -ge 1 ] && [ "$FOUND_CHILD" -ge 1 ]; then
  echo "PASS: Found both parent & child messages in serial output"
  exit 0
else
  echo "FAIL: Did not find expected fork output in serial logs"
  echo "--- Serial Output (tail) ---"
  tail -n 200 "$SERIAL_LOG" || true
  exit 1
fi
