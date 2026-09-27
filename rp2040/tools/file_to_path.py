#!/usr/bin/env python3
"""Print the Babel directory path whose `file` has exactly the input bytes."""
from pathlib import Path
import argparse

ALPHABET = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789! &()-_+"
BASE = len(ALPHABET) ** 2


def file_to_components(data: bytes) -> list[str]:
    index = sum((byte + 1) * (256 ** position) for position, byte in enumerate(data))
    result: list[str] = []
    while index:
        index -= 1
        component = index % BASE
        result.append(ALPHABET[component // len(ALPHABET)] + ALPHABET[component % len(ALPHABET)])
        index //= BASE
    return list(reversed(result))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("file", type=Path)
    args = parser.parse_args()
    data = args.file.read_bytes()
    if len(data) > 4096:
        parser.error("firmware supports files up to 4096 bytes")
    components = file_to_components(data)
    if len(components) > 2700:
        parser.error("path exceeds firmware depth limit")
    print("disk" + "".join(f"/{part}" for part in components) + "/file")


if __name__ == "__main__":
    main()
