#!/bin/bash
# Script para ejecutar SO_Descentralizado con shell interactivo

echo "Iniciando SO_Descentralizado con shell interactivo..."
echo ""
echo "IMPORTANTE: Una vez que aparezca el prompt 'myos>', escribe directamente en la ventana."
echo ""
echo "Comandos disponibles: help, uname, meminfo, echo, uptime, version, about, clear, reboot"
echo ""
echo "Presiona Ctrl+C para salir de QEMU"
echo ""
echo "=========================================="
echo ""

cd "$(dirname "$0")"

# Opción 1: Modo curses (mejor para interacción de teclado)
qemu-system-x86_64 \
    -cdrom kernel.iso \
    -m 256M \
    -display curses \
    -serial stdio

# Opción 2: Si curses no funciona, usa modo gráfico
# qemu-system-x86_64 \
#     -cdrom kernel.iso \
#     -m 256M \
#     -vga std \
#     -serial file:serial.log
