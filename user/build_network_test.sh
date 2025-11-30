#!/bin/bash
# Build network_test user program

echo "Building network_test..."

# Compile
gcc -m64 -static -nostdlib -fno-builtin -fcf-protection=none \
    -fno-stack-protector -O0 -c user/network_test.c -o /tmp/network_test.o

# Link
ld -Ttext=0x400000 -nostdlib /tmp/network_test.o -o build_user/network_test.elf

# Convert to header
xxd -i build_user/network_test.elf > kernel/user_network_test_bin.h

echo "network_test built and embedded"
