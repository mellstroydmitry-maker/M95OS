#!/bin/bash
set -e
nasm -f elf32 boot.s -o boot.o
gcc -m32 -ffreestanding -fno-pic -fno-stack-protector -mno-sse -mno-sse2 -mno-mmx -c kernel.c -o kernel.o -O2 -Wall
gcc -m32 -ffreestanding -fno-pic -fno-stack-protector -mno-sse -mno-sse2 -mno-mmx -c gui.c -o gui.o -O2 -Wall
gcc -m32 -ffreestanding -fno-pic -fno-stack-protector -mno-sse -mno-sse2 -mno-mmx -c mouse.c -o mouse.o -O2 -Wall
gcc -m32 -ffreestanding -fno-pic -fno-stack-protector -mno-sse -mno-sse2 -mno-mmx -c keyboard.c -o keyboard.o -O2 -Wall
gcc -m32 -ffreestanding -fno-pic -fno-stack-protector -mno-sse -mno-sse2 -mno-mmx -c settings_app.c -o settings_app.o -O2 -Wall
gcc -m32 -ffreestanding -fno-pic -fno-stack-protector -mno-sse -mno-sse2 -mno-mmx -c games_app.c -o games_app.o -O2 -Wall
gcc -m32 -ffreestanding -fno-pic -fno-stack-protector -mno-sse -mno-sse2 -mno-mmx -c paint_app.c -o paint_app.o -O2 -Wall
gcc -m32 -ffreestanding -fno-pic -fno-stack-protector -mno-sse -mno-sse2 -mno-mmx -c craft_app.c -o craft_app.o -O2 -Wall
gcc -m32 -ffreestanding -fno-pic -fno-stack-protector -mno-sse -mno-sse2 -mno-mmx -c m95pkg.c -o m95pkg.o -O2 -Wall
ld -m elf_i386 -T linker.ld -o m95os.bin boot.o kernel.o gui.o mouse.o keyboard.o settings_app.o games_app.o paint_app.o craft_app.o m95pkg.o
rm -rf isodir
mkdir -p isodir/boot/grub isodir/packages
cp m95os.bin isodir/boot/m95os.bin
cat << 'GRUBEOF' > isodir/boot/grub/grub.cfg
set timeout=0
set default=0
menuentry "M95OS pre-release" {
multiboot /boot/m95os.bin
GRUBEOF
for pkg in packages/*.m95; do
if [ -f "$pkg" ]; then
bname=$(basename "$pkg")
cp "$pkg" "isodir/packages/$bname"
echo "    module /packages/$bname" >> isodir/boot/grub/grub.cfg
fi
done
echo "    boot" >> isodir/boot/grub/grub.cfg
echo "}" >> isodir/boot/grub/grub.cfg
grub-mkrescue -o m95os.iso isodir
echo "M95OS pre-release built successfully!"
