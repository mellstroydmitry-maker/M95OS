#!/usr/bin/env python3
import sys
if len(sys.argv) < 5:
    print("Usage: python3 sdk/pack_m95.py <code.bin> <pkg_name> <author> <packages/out.m95>")
    sys.exit(1)
with open(sys.argv[1], "rb") as f:
    code = f.read()
header = b"M95P" + sys.argv[2].encode("utf-8")[:23].ljust(24, b"\x00") + sys.argv[3].encode("utf-8")[:23].ljust(24, b"\x00")
with open(sys.argv[4], "wb") as f:
    f.write(header + code)
print(f"Created package {sys.argv[4]} ({len(header) + len(code)} bytes)")
