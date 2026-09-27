#!/usr/bin/env python3
"""Generate the bytes represented by a USB of Babel directory path."""
from pathlib import Path
import argparse

ALPHABET = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789! &()-_+"
BASE = len(ALPHABET) ** 2


def components_to_file(components: list[str]) -> bytes:
    index = 0
    for name in components:
        if len(name) != 2 or any(character not in ALPHABET for character in name):
            raise ValueError(f"invalid directory name: {name!r}")
        digit = ALPHABET.index(name[0]) * len(ALPHABET) + ALPHABET.index(name[1])
        index = index * BASE + digit + 1

    output = bytearray()
    while index:
        output.append((index - 1) & 0xFF)
        index = (index - 1) >> 8
    return bytes(output)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("path", help="For example: disk/AA/B7/file or /AA/B7")
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    parts = [part for part in args.path.split("/") if part]
    if parts and parts[0] == "disk":
        parts.pop(0)
    if parts and parts[-1] == "file":
        parts.pop()
    args.output.write_bytes(components_to_file(parts))


if __name__ == "__main__":
    main()
