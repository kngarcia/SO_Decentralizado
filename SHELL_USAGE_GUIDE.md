# 🖥️ GUÍA DE USO DEL SHELL INTERACTIVO

## 📋 Índice
- [Cómo Iniciar el Shell](#cómo-iniciar-el-shell)
- [Comandos Disponibles](#comandos-disponibles)
- [Ejemplos de Uso](#ejemplos-de-uso)
- [Atajos de Teclado](#atajos-de-teclado)
- [Troubleshooting](#troubleshooting)

---

## 🚀 Cómo Iniciar el Shell

### Opción 1: Modo Interactivo con QEMU (Recomendado)

```bash
# En PowerShell (Windows con WSL):
wsl bash -c "cd /mnt/c/Users/Nicolas/Downloads/proyectoj/SO_Decentralizado && qemu-system-x86_64 -cdrom kernel.iso -m 256M -serial stdio -display none"

# En Linux/Mac:
cd SO_Decentralizado
qemu-system-x86_64 -cdrom kernel.iso -m 256M -serial stdio -display none
```

**Salida esperada:**
```
START
MBI
...
[kmain] ML subsystem operational (100%)

[kmain] Starting interactive shell...

========================================
  Welcome to SO_Descentralizado!
  Interactive Shell v1.0
========================================

Type 'help' for available commands.

myos> _
```

### Opción 2: Con Interfaz Gráfica VGA

```bash
wsl bash -c "cd /mnt/c/Users/Nicolas/Downloads/proyectoj/SO_Decentralizado && qemu-system-x86_64 -cdrom kernel.iso -m 256M -vga std"
```

Esto abrirá una ventana con la salida VGA del sistema.

---

## 📝 Comandos Disponibles

### `help`
Muestra la lista de comandos disponibles.

```
myos> help

Available commands:
  help      - Show this help message
  clear     - Clear the screen
  uname     - Show system information
  meminfo   - Show memory statistics
  echo      - Echo text to console
  uptime    - Show system uptime
  version   - Show kernel version
  reboot    - Reboot the system
  about     - About this OS
```

---

### `clear`
Limpia la pantalla (imprime múltiples líneas nuevas).

```
myos> clear
```

---

### `uname`
Muestra información del sistema operativo.

```
myos> uname

SO_Descentralizado v1.0.0
Architecture: x86_64
Kernel: 64-bit long mode
Build: November 30, 2025
```

---

### `meminfo`
Muestra estadísticas de memoria del sistema.

```
myos> meminfo

Memory Information:
  Total physical memory: 128 MB
  Free physical memory:  96 MB
  Memory management: Physical + Virtual + Paging
  Page size: 4KB
```

---

### `echo [texto]`
Imprime el texto proporcionado a la consola.

```
myos> echo Hello World!

Hello World!

myos> echo Sistema operativo funcionando correctamente

Sistema operativo funcionando correctamente
```

---

### `uptime`
Muestra el tiempo de ejecución y estado de los subsistemas.

```
myos> uptime

System uptime: Running since boot
Subsystems active:
  [✓] Memory Management
  [✓] Process Manager
  [✓] Scheduler (preemptive)
  [✓] Syscall Interface (23 syscalls)
  [✓] Network Stack (E1000)
  [✓] ML Subsystem
  [✓] WASM3 Runtime
  [✓] Framebuffer Driver
```

---

### `version`
Muestra información detallada de la versión del kernel.

```
myos> version

SO_Descentralizado Kernel v1.0.0
Compilation: GCC 64-bit, -O2 optimized
Features:
  • 64-bit x86-64 long mode
  • Preemptive multitasking
  • Copy-on-Write fork()
  • ELF loader (ring-3 execution)
  • IPC message passing
  • ML/DL (Linear Regression)
  • Ad hoc networking
  • WASM3 runtime
```

---

### `about`
Muestra información general sobre el sistema operativo.

```
myos> about

========================================
  SO_Descentralizado v1.0.0
  Decentralized Operating System
========================================

A 64-bit operating system with:
  • Distributed computing capabilities
  • Machine Learning integration
  • WebAssembly support
  • Modern memory management
  • Ad hoc networking

Completed: 15/15 requirements (100%)
Status: Fully operational

Developed: 2025
License: Educational/Research
```

---

### `reboot`
Reinicia el sistema operativo.

```
myos> reboot

Rebooting system...
```

---

## 🎯 Ejemplos de Uso

### Sesión de Ejemplo Completa

```
myos> uname
SO_Descentralizado v1.0.0
Architecture: x86_64
Kernel: 64-bit long mode
Build: November 30, 2025

myos> meminfo
Memory Information:
  Total physical memory: 128 MB
  Free physical memory:  96 MB
  Memory management: Physical + Virtual + Paging
  Page size: 4KB

myos> uptime
System uptime: Running since boot
Subsystems active:
  [✓] Memory Management
  [✓] Process Manager
  [✓] Scheduler (preemptive)
  [✓] Syscall Interface (23 syscalls)
  [✓] Network Stack (E1000)
  [✓] ML Subsystem
  [✓] WASM3 Runtime
  [✓] Framebuffer Driver

myos> echo Probando el sistema operativo!
Probando el sistema operativo!

myos> version
SO_Descentralizado Kernel v1.0.0
...

myos> about
========================================
  SO_Descentralizado v1.0.0
  Decentralized Operating System
========================================
...
```

---

## ⌨️ Atajos de Teclado

| Tecla | Acción |
|-------|--------|
| `Enter` | Ejecutar comando |
| `Backspace` | Borrar último carácter |
| `Shift + [letra]` | Letra mayúscula |
| Cualquier carácter imprimible | Agregar al buffer de comando |

**Nota**: Actualmente no hay soporte para:
- Flechas arriba/abajo (historial de comandos)
- Ctrl+C (cancelar comando)
- Tab (autocompletado)

---

## 🔍 Troubleshooting

### El shell no aparece

**Solución**: Verifica que QEMU está ejecutando correctamente:

```bash
wsl bash -c "cd /mnt/c/Users/Nicolas/Downloads/proyectoj/SO_Decentralizado && timeout 15 qemu-system-x86_64 -cdrom kernel.iso -m 256M -serial stdio -display none"
```

Deberías ver el banner del shell después del mensaje `[kmain] Starting interactive shell...`

---

### No puedo escribir nada

**Causa**: El keyboard polling puede necesitar un momento para inicializar.

**Solución**: Espera 1-2 segundos después de que aparezca el prompt `myos>`, luego intenta escribir.

---

### Los caracteres aparecen duplicados o incorrectos

**Causa**: Problema con el mapeo de scancode del teclado.

**Solución**: Esto puede ocurrir en algunos emuladores. Usa QEMU directamente con `-serial stdio` para mejor compatibilidad.

---

### El sistema se congela después de un comando

**Causa**: El comando puede estar esperando entrada adicional o hay un bug.

**Solución**: 
1. Intenta presionar `Enter` nuevamente
2. Si no responde, termina QEMU (`Ctrl+C` en la terminal que lo ejecutó)
3. Reinicia: `wsl bash -c "cd ... && qemu-system-x86_64 -cdrom kernel.iso ..."`

---

### Quiero salir del shell

**Solución**: 
- En modo serial (stdio): Presiona `Ctrl+C` en la terminal de PowerShell/Linux
- En modo gráfico: Cierra la ventana de QEMU
- Dentro del sistema: Usa el comando `reboot`

---

## 🎓 Comandos para Verificar Funcionalidad

### Test Rápido de Subsistemas

```bash
myos> uptime
# Verifica que todos los subsistemas muestren [✓]

myos> meminfo
# Verifica que haya memoria disponible

myos> version
# Verifica que todas las features estén listadas

myos> echo Test successful!
# Verifica que el echo funcione correctamente
```

---

## 📊 Información Técnica

### Arquitectura del Shell

```
Keyboard Input (scancode) 
    ↓
Scancode → ASCII mapping
    ↓
Command buffer (128 chars max)
    ↓
Command parser
    ↓
Built-in command execution
    ↓
Serial output (COM1)
```

### Limitaciones Actuales

1. **Sin historial de comandos**: No se guardan comandos previos
2. **Sin autocompletado**: No hay sugerencias de comandos
3. **Buffer limitado**: Máximo 128 caracteres por comando
4. **Sin pipes/redirection**: No hay soporte para `|`, `>`, `<`
5. **Sin variables de entorno**: No hay `$PATH`, `$HOME`, etc.
6. **Sin scripting**: No se pueden ejecutar scripts

### Comandos Internos (Built-ins)

Todos los comandos son **built-in** (integrados en el kernel), no hay ejecutables externos en disco.

---

## 🚀 Próximas Mejoras Planeadas

- [ ] Historial de comandos (arriba/abajo)
- [ ] Autocompletado con Tab
- [ ] Sistema de archivos básico con `ls`, `cat`, `mkdir`
- [ ] Ejecutar programas ELF desde disco
- [ ] Soporte para pipes y redirection
- [ ] Variables de entorno
- [ ] Shell scripting básico

---

## 📞 Soporte

Para reportar bugs o sugerir mejoras:
1. Verifica que el sistema esté actualizado (últimos commits en GitHub)
2. Reproduce el problema con los comandos exactos
3. Captura el log completo (`-serial file:bug.log`)
4. Documenta el comportamiento esperado vs. actual

---

**¡Disfruta explorando SO_Descentralizado!** 🎉
