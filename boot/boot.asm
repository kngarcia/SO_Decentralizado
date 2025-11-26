; boot/boot.asm
; placeholder: file included for structure.
; We use GRUB multiboot loader. This file is not required by GRUB but kept for compatibility.
; (Puede contener un mensaje o un header alternativo si quieres experimentar.)

BITS 16
org 0x7c00

jmp $.boot_msg
times 510-($-$$) db 0
dw 0xaa55

.boot_msg:
    db "MYOS placeholder boot sector - GRUB used instead",0
