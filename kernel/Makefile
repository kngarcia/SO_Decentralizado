# kernel/Makefile - build kernel ELF 64-bit
# Booteable on any UEFI/BIOS system (VM, physical hardware, cloud)
CSRC := $(shell find . -name '*.c')
ASMS := start.S arch/x86/interrupts.S
OBJS := $(CSRC:.c=.o) $(ASMS:.S=.o)

# x86-64 compilation flags
CFLAGS := -m64 -ffreestanding -O2 -Wall -Wextra -fno-asynchronous-unwind-tables -fno-stack-protector
ASFLAGS := -m64

all: kernel.bin

%.o: %.c
	gcc $(CFLAGS) -c $< -o $@

%.o: %.S
	gcc $(ASFLAGS) -c $< -o $@

kernel.bin: $(OBJS) linker.ld
	ld -m elf_x86_64 -T linker.ld -o kernel.elf $(OBJS)
	# kernel.elf is ELF64, bootable by GRUB/Multiboot
	mv kernel.elf ../kernel.elf

clean:
	rm -f $(OBJS) kernel.elf ../kernel.elf

.PHONY: all clean

