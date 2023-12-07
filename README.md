# CamOS kernel
## What's CamOS
CamOS is a 64 bit toy/hobby operating system, i'm doing this just because i wanted to know more on how an operating system works, and because.. why not.
This is meant to be a command line operating system so it's "hardwired" to be so.

## Architecture
CamOS uses its own UEFI bootloader LeoBoot, which sets up a minimal environment for the kernel to keep initializing the machine.
Once the bootloader jumps to the kernel (which is statically loaded at a high virtual address, i can't remember which address and i'm too lazy to go check) the kernel
bootstraps itself to an environment suitable to run user code.

## No rules
My code is totally different from anyone else's. I don't like rules so everything here is not what you expect it to be. My code could be messy, clean, unefficient, the most
beautiful code you ever seen, idk.. it's up to you, this is my style :)

## How to compile
INPUT_FILES = $(shell find src -not -name "isr.c" -name "*.c")
gcc -c -ffreestanding -nostdlib -O0 -m64 -no-pie -Isrc $(INPUT_FILES)
gcc -c -ffreestanding -nostdlib -O0 -m64 -no-pie -mgeneral-regs-only -Isrc src/int/*.c
ld -T linker.ld *.o -o leokernel.elf

the linker script (linker.ld) is not present here, i'll upload it later on