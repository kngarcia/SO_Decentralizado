#!/usr/bin/env bash
# Quick QEMU integration smoke-test for embedded user ELF
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

echo "Building ISO..."
make iso >/dev/null

mkdir -p tmp
SERIAL_LOG="tmp/qemu_serial.log"
rm -f "$SERIAL_LOG"

echo "Running QEMU for 8s (capturing serial to $SERIAL_LOG)"
if ! command -v qemu-system-x86_64 >/dev/null 2>&1; then
  echo "qemu-system-x86_64 not found in PATH — please install QEMU to run this test."
  exit 2
fi

# Run QEMU for a short time and capture serial output to file
timeout 8s qemu-system-x86_64 -cdrom myos.iso -m 512M -serial file:$SERIAL_LOG >/dev/null 2>&1 || true

echo "QEMU finished; scanning logs for user message..."
if grep -q "Hello from ring-3" "$SERIAL_LOG"; then
  echo "PASS: Found user hello in serial output"
  exit 0
else
  echo "FAIL: user hello not found in serial output"
  printf "--- serial output (last 200 lines) ---\n"
  tail -n 200 "$SERIAL_LOG" || true
  exit 1
fi
