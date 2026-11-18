#!/bin/bash
set -e
nasm -f elf32 boot.s -o boot.o
gcc -m32 -ffreestanding -fno-pic -fno-stack-protector -mno-sse -mno-sse2 -mno-mmx -c kernel.c -o kernel.o -O2 -Wall
gcc -m32 -ffreestanding -fno-pic -fno-stack-protector -mno-sse -mno-sse2 -mno-mmx -c gui.c -o gui.o -O2 -Wall
gcc -m32 -ffreestanding -fno-pic -fno-stack-protector -mno-sse -mno-sse2 -mno-mmx -c mouse.c -o mouse.o -O2 -Wall
gcc -m32 -ffreestanding -fno-pic -fno-stack-protector -mno-sse -mno-sse2 -mno-mmx -c keyboard.c -o keyboard.o -O2 -Wall
ld -m elf_i386 -T linker.ld -o m95os.bin boot.o kernel.o gui.o mouse.o keyboard.o
mkdir -p isodir/boot/grub
cp m95os.bin isodir/boot/m95os.bin
cp grub.cfg isodir/boot/grub/grub.cfg
grub-mkrescue -o m95os.iso isodir
echo "M95OS built successfully: m95os.iso"
