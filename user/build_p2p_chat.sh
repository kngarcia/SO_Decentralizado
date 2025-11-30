#!/bin/bash
# Build P2P Chat application

echo "Building P2P Chat application..."

gcc -m64 -static -nostdlib -fno-stack-protector \
    -fcf-protection=none -O2 \
    -o app_p2p_chat.elf app_p2p_chat.c

if [ $? -eq 0 ]; then
    echo "Build successful: app_p2p_chat.elf"
    
    # Generate header file for embedding in kernel
    xxd -i app_p2p_chat.elf > ../kernel/app_p2p_chat_bin.h
    echo "Generated header: kernel/app_p2p_chat_bin.h"
else
    echo "Build failed"
    exit 1
fi
