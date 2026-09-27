# Babel USB Pico

Turn a Raspberry Pi Pico into a read-only, seemingly endless [Library of
Babel](https://libraryofbabel.info/) over USB. Every directory contains 4,900
more directories and a file named `file`; the path determines that file's
bytes. This is the RP2040 port of [p2r3's USB of Babel](https://github.com/p2r3/babel-usb).

**[Get the Pico UF2](https://github.com/palermostest25/Babel-USB-Pico/releases/tag/v0.1.0)** · **[Pico source and full instructions](rp2040/)**

## Flash a Pi Pico

1. Download `Babel-USB-Pico.uf2` from the [v0.1.0 release](https://github.com/palermostest25/Babel-USB-Pico/releases/tag/v0.1.0).
2. Hold **BOOTSEL** while connecting the Pico to your computer.
3. Copy the UF2 to the `RPI-RP2` drive. After it reboots, unplug and reconnect the Pico.

Open **USB of Babel** with an MTP-capable file browser. It appears as a portable
device, not a normal USB disk. macOS needs an MTP client because Finder does not
mount generic MTP devices.

The firmware is built for the original Raspberry Pi Pico (`PICO_BOARD=pico`).
No extra wiring or storage is needed. It has been cross-compiled and its
byte/path arithmetic has passed host tests; physical Pico testing is still
pending.

## Find your file in the Babel tree

From the repository root, run:

```sh
python3 rp2040/tools/file_to_path.py my-file.bin
```

The tool prints a path beginning with `disk/` and ending in `/file`. Follow
that path on the Pico and copy `file` back to your computer. The firmware
supports files up to 4 KiB, though paths for large files are too deep for most
file browsers to navigate comfortably. The Pico filesystem is read-only and
never writes its flash.

## Build from source

```sh
cd rp2040
./build.sh
```

The build downloads pinned Pico SDK and TinyUSB dependencies and produces
`rp2040/build/babel_usb.uf2`. See the [Pico guide](rp2040/README.md) for
toolchain requirements, tests, and technical notes.

## Original ESP32-S3 project

The original ESP32-S3 firmware remains at the repository root. Its setup and
usage instructions are preserved in the [ESP32-S3 guide](docs/esp32-s3.md).
Credit for the original concept and implementation belongs to
[p2r3](https://github.com/p2r3/babel-usb); the Pico port uses
[TinyUSB](https://github.com/hathach/tinyusb) and the
[Pico SDK](https://github.com/raspberrypi/pico-sdk).
