# Third-party notices

This is a clean RP2040 implementation inspired by p2r3/portalrunner's
`babel-usb` project. The original project's MTP integration was derived from
work by RigoLigoRLC and Ennebi Elettronica.

The build downloads these upstream dependencies:

- Raspberry Pi Pico SDK 2.3.1 — BSD-3-Clause
- TinyUSB 0.21.0 — MIT

The MTP callback organization and protocol data structures follow TinyUSB's
MIT-licensed MTP device example. Dependency license texts are present in their
respective checkouts after running `bootstrap.sh`.
