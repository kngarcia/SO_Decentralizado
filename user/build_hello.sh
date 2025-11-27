#!/usr/bin/env bash
# Script to compile user/hello.c and generate kernel header
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

echo "Compiling user/hello.c..."
gcc -m64 -static -nostdlib -fno-builtin -fcf-protection=none \
    -fno-stack-protector -O0 -c user/hello.c -o /tmp/hello.o

echo "Linking hello.elf..."
ld -Ttext=0x400000 -nostdlib /tmp/hello.o -o build_user/hello_nocet.elf

echo "Generating kernel header..."
xxd -i build_user/hello_nocet.elf > kernel/user_hello_bin_nocet.h

echo "Done! Lines in header: $(wc -l < kernel/user_hello_bin_nocet.h)"
echo "Now rebuild the kernel with: make -C kernel clean && make -C kernel && make iso"
