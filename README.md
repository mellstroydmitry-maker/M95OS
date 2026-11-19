<p align="center">
  <img src="logo.png" alt="M95OS" width="110">
</p>

<h1 align="center">M95OS</h1>

<p align="center">
  A 32-bit x86 hobby operating system written from scratch in C and Assembly, styled after Windows 95.
</p>

## Features

* **GUI & Graphics:** 800x600 32-bit VBE framebuffer, double buffering via `rep movsl`, overlapping windows, taskbar, Start menu, right-click context menu, and custom UI themes (`DarkWin`, `FlatUI`, cursor colors).
* **Localization:** Built-in English and Russian (UTF-8 8x14 bitmap font and PS/2 keyboard layouts).
* **Networking & Browser:**
  * Bare-metal PCI driver for **Realtek RTL8139** (`10EC:8139`).
  * Custom **Ethernet II / ARP / IPv4 / UDP** stack.
  * **M95 Explorer:** lightweight HTML parser (`<h1>`, `<p>`, `<hr>`, `<a href>`) with URL bar, scrolling, and real internet access via `web_gateway.py`.
* **Apps & Games:**
  * **M95 Craft:** isometric block-building sandbox.
  * **M95 Paint:** bitmap canvas editor with palette, brush sizes, and eraser.
  * **TinyGL 3D:** software 3D cube renderer with triangle rasterization.
  * **M95 Arcade:** Minesweeper, Snake, Breakout, Tetris, and Pong.
  * **M95 Shell & Notepad.**
* **Security & Packages (`[EXP]`):**
  * Hardware ID generation (`CPUID` + RAM) with admin password hash stored directly to raw disk sector via **ATA PIO**.
  * Experimental `m95pkg` package manager + C SDK (`sdk/m95_sdk.h`, `sdk/pack_m95.py`) for loading external `.m95` binaries (`xeyes`, `rain`, `radar`).

## Build & Run

Requires `nasm`, `gcc` (with 32-bit multilib), `ld`, `grub-mkrescue`, `xorriso`, and `qemu-system-i386`.

```bash
chmod +x build.sh
./build.sh

# Optional: create a 1MB raw disk image to persist the admin password
dd if=/dev/zero of=m95_disk.img bs=1M count=1

# Optional: start the host UDP-to-HTTP gateway for M95 Explorer
python3 web_gateway.py &

# Boot in QEMU
qemu-system-i386 -m 256M -vga std \
  -drive file=m95_disk.img,format=raw \
  -cdrom m95os.iso \
  -netdev user,id=net0,hostfwd=udp::5555-:5555 \
  -device rtl8139,netdev=net0 \
  -display gtk
```

## License

GPL-3.0
