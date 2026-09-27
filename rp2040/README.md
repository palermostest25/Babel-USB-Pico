# USB of Babel for Raspberry Pi Pico (RP2040)

This firmware turns a Raspberry Pi Pico into a read-only MTP device containing
an effectively endless directory tree. Every directory contains 4,900 more
directories plus one file named `file`. The path deterministically encodes the
file bytes, so every file up to the firmware's 4 KiB working limit has a
location.

This is a clean RP2040 port inspired by p2r3/portalrunner's original ESP32-S3
project. It uses the RP2040's native USB controller and upstream TinyUSB MTP;
no external wiring, storage, or RTOS is required.

## Hardware

- Raspberry Pi Pico or another RP2040 board with a native USB connector
- A data-capable micro-USB cable

The default build target is the original Raspberry Pi Pico (`PICO_BOARD=pico`).

## Build

Requirements: Git, CMake 3.13+, Python 3, and a complete Arm GNU embedded
toolchain with Newlib. On macOS, the `gcc-arm-embedded` Homebrew cask supplies
the complete toolchain; the `arm-none-eabi-gcc` formula alone does not.

```sh
./build.sh
```

The first build downloads pinned copies of Pico SDK 2.3.1 and TinyUSB 0.21.0.
The result is `build/babel_usb.uf2`.

To use an existing SDK/toolchain instead:

```sh
PICO_SDK_PATH=/path/to/pico-sdk \
PICO_TINYUSB_PATH=/path/to/tinyusb \
cmake -S . -B build -DPICO_BOARD=pico
cmake --build build --parallel
```

TinyUSB must be version 0.21.0 or newer because older Pico SDK bundles do not
contain its MTP device class.

## Flash

This fork includes a compiled UF2 at `firmware/babel_usb.uf2`; you can use it
without building the source. A local build produces `build/babel_usb.uf2`.

1. Unplug the Pico.
2. Hold **BOOTSEL** while plugging it in.
3. Copy either UF2 to the `RPI-RP2` drive.
4. Let the Pico reboot, then unplug and reconnect it once.
5. Open **USB of Babel** in the operating system's MTP/file browser.

This is MTP, not USB mass storage. It appears as a portable device rather than
a normal disk. Linux desktops may require an MTP/GVfs package. macOS does not
mount generic MTP devices in Finder; use an MTP client such as OpenMTP or an
Android File Transfer-compatible client.

The onboard LED blinks every 250 ms while disconnected, every second while
mounted, and every 2.5 seconds while USB is suspended.

## Find a file

```sh
python3 tools/file_to_path.py picture.png
```

The script prints a path such as `disk/AA/B7/.../file`. Follow that path on the
Pico and copy `file` back to the computer. Long inputs produce very deep paths;
the 4 KiB firmware limit is theoretical, and host file browsers become
impractical much sooner.

To verify or regenerate a path locally:

```sh
python3 tools/path_to_file.py 'disk/AA/B7/file' recovered.bin
```

## Tests

```sh
./tests/run_tests.sh
```

These tests run the exact byte/path arithmetic on the host and round-trip a
binary fixture through both helper tools.

## RP2040 port notes

- Bare-metal polling replaces the ESP32-S3/FreeRTOS task.
- Pico's unique flash ID supplies the MTP/USB serial number.
- Object handle lists are streamed in 512-byte chunks, so a 4,901-entry folder
  does not consume a 20 KiB handle array in RAM.
- A path stack makes normal back-navigation deterministic; stale MTP handles
  from a different branch remain inherently ambiguous because an infinite tree
  cannot have globally unique 32-bit MTP handles.
- The filesystem is intentionally read-only and never writes Pico flash.

## Credits

- Original concept and ESP32-S3 implementation: [p2r3/babel-usb](https://github.com/p2r3/babel-usb)
- USB device stack: [TinyUSB](https://github.com/hathach/tinyusb)
- RP2040 support: [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk)

See `THIRD_PARTY.md` for license details.
