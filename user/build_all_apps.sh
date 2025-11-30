#!/bin/bash
# Build all distributed applications

echo "=== Building All Distributed Applications ==="

# Build P2P Chat
echo ""
echo "1/3: Building P2P Chat..."
./build_p2p_chat.sh
if [ $? -ne 0 ]; then
    echo "FAILED: P2P Chat"
    exit 1
fi

# Build File Share
echo ""
echo "2/3: Building File Share..."
./build_file_share.sh
if [ $? -ne 0 ]; then
    echo "FAILED: File Share"
    exit 1
fi

# Build ML Demo
echo ""
echo "3/3: Building ML Demo..."
./build_ml_demo.sh
if [ $? -ne 0 ]; then
    echo "FAILED: ML Demo"
    exit 1
fi

echo ""
echo "=== All applications built successfully ==="
echo ""
echo "Generated files:"
echo "  - app_p2p_chat.elf"
echo "  - app_file_share.elf"
echo "  - app_ml_demo.elf"
echo ""
echo "Embedded headers:"
echo "  - ../kernel/app_p2p_chat_bin.h"
echo "  - ../kernel/app_file_share_bin.h"
echo "  - ../kernel/app_ml_demo_bin.h"
