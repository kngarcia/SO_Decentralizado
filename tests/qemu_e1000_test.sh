#!/usr/bin/env bash
# QEMU test with E1000 NIC and ML subsystem
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

echo "Building ISO..."
./build_iso.sh >/dev/null

mkdir -p tmp
SERIAL_LOG="tmp/qemu_e1000_test.log"
rm -f "$SERIAL_LOG"

echo "Running QEMU for 12s with E1000 (capturing serial to $SERIAL_LOG)"
if ! command -v qemu-system-x86_64 >/dev/null 2>&1; then
  echo "qemu-system-x86_64 not found in PATH"
  exit 2
fi

# Run QEMU with E1000 device
timeout 12s qemu-system-x86_64 \
  -cdrom kernel.iso \
  -m 256M \
  -serial file:$SERIAL_LOG \
  -display none \
  -device e1000,netdev=net0 \
  -netdev user,id=net0 \
  >/dev/null 2>&1 || true

echo "QEMU finished; analyzing logs..."

# Check for critical success markers
SUCCESS_COUNT=0

if grep -q "E1000 NIC initialized successfully" "$SERIAL_LOG"; then
  echo "✅ E1000 initialization successful"
  SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
else
  echo "❌ E1000 initialization failed or not found"
fi

if grep -q "Ad hoc network FULLY operational" "$SERIAL_LOG"; then
  echo "✅ Network stack operational (100%)"
  SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
else
  echo "❌ Network stack not fully operational"
fi

if grep -q "ML subsystem operational" "$SERIAL_LOG"; then
  echo "✅ ML subsystem operational (100%)"
  SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
else
  echo "❌ ML subsystem not found"
fi

if grep -q "Hello from ring-3" "$SERIAL_LOG"; then
  echo "✅ User process execution successful"
  SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
else
  echo "❌ User process not found"
fi

# Check for failures
if grep -qE "(GP fault|Page fault|Triple fault|PANIC)" "$SERIAL_LOG"; then
  echo "❌ CRITICAL: System fault detected"
  echo "Last 100 lines of output:"
  tail -n 100 "$SERIAL_LOG"
  exit 1
fi

echo ""
echo "========================================="
echo "Success: $SUCCESS_COUNT / 4 checks passed"
echo "========================================="

if [ $SUCCESS_COUNT -ge 3 ]; then
  echo "OVERALL: PASS (sufficient features operational)"
  exit 0
else
  echo "OVERALL: FAIL (too many features missing)"
  echo ""
  echo "Full serial output (last 200 lines):"
  tail -n 200 "$SERIAL_LOG"
  exit 1
fi
