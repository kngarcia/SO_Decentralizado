#!/bin/bash
# Build P2P File Share application

echo "Building P2P File Share application..."

gcc -m64 -static -nostdlib -fno-stack-protector \
    -fcf-protection=none -O2 \
    -o app_file_share.elf app_file_share.c

if [ $? -eq 0 ]; then
    echo "Build successful: app_file_share.elf"
    
    # Generate header file for embedding in kernel
    xxd -i app_file_share.elf > ../kernel/app_file_share_bin.h
    echo "Generated header: kernel/app_file_share_bin.h"
else
    echo "Build failed"
    exit 1
fi
