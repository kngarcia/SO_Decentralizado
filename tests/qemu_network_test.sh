#!/bin/bash
# Test script for Phase 3 networking functionality
# Tests E1000 NIC driver, network stack, and P2P overlay

set -e

echo "Building kernel with Phase 3 networking..."
cd kernel
make clean
make -j8
cd ..

echo "Creating bootable ISO..."
cp kernel.elf isodir/boot/
grub-mkrescue -o myos.iso isodir/

echo "Starting QEMU with networking (E1000 NIC)..."
SERIAL_LOG="tmp/qemu_network_test.log"
rm -f "$SERIAL_LOG"

# Run QEMU with E1000 network adapter
timeout 10s qemu-system-x86_64 \
    -cdrom myos.iso \
    -m 512M \
    -serial file:"$SERIAL_LOG" \
    -display none \
    -netdev user,id=net0 \
    -device e1000,netdev=net0,mac=52:54:00:12:34:56 \
    || true

echo ""
echo "=== Checking network initialization logs ==="
cat "$SERIAL_LOG"

echo ""
echo "=== Test Results ==="

# Check for network initialization
if grep -q "\[e1000\] Initialization complete" "$SERIAL_LOG"; then
    echo "✓ PASS: E1000 NIC driver initialized"
else
    echo "✗ FAIL: E1000 NIC driver not initialized"
    exit 1
fi

# Check for network stack initialization
if grep -q "\[kmain\] Network stack initialized" "$SERIAL_LOG"; then
    echo "✓ PASS: Network stack initialized"
else
    echo "✗ FAIL: Network stack not initialized"
    exit 1
fi

# Check for IP configuration
if grep -q "Network configured: 192.168.1.2" "$SERIAL_LOG"; then
    echo "✓ PASS: IP address configured"
else
    echo "✗ FAIL: IP address not configured"
    exit 1
fi

# Check for P2P initialization
if grep -q "\[p2p\] Initialized" "$SERIAL_LOG"; then
    echo "✓ PASS: P2P network initialized"
else
    echo "✗ FAIL: P2P network not initialized"
    exit 1
fi

echo ""
echo "=== All Phase 3 networking tests passed! ==="
