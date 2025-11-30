#!/bin/bash
# Build ML Demo application

echo "Building ML Demo application..."

gcc -m64 -static -nostdlib -fno-stack-protector \
    -fcf-protection=none -O2 \
    -o app_ml_demo.elf app_ml_demo.c

if [ $? -eq 0 ]; then
    echo "Build successful: app_ml_demo.elf"
    
    # Generate header file for embedding in kernel
    xxd -i app_ml_demo.elf > ../kernel/app_ml_demo_bin.h
    echo "Generated header: kernel/app_ml_demo_bin.h"
else
    echo "Build failed"
    exit 1
fi
