# M5Unit-TrackBall Internal Firmware

## Overview

### SKU: U243

Internal firmware for the M5Stack **Unit TrackBall**. This repository contains the STM32G030F6P6 application project, the IAP bootloader project, released firmware images, and the I2C control protocol document.

Unit TrackBall is a trackball input unit built around the MOT6010 optical sensor. It works as an I2C slave: the host reads X/Y motion, accumulated motion and status registers, and can configure the axis mapping, key timing, blue LED brightness and the persistent I2C address. The on-board key supports single-click, double-click and long-press events.

## Related Link

- [Unit TrackBall & Datasheet](https://docs.m5stack.com/en/products/sku/U243)
- [I2C Control Protocol (Chinese)](protocol/UNIT_TRACKBALL_I2C_CONTROL_PROTOCOL.md)
- [I2C Control Protocol (English)](protocol/UNIT_TRACKBALL_I2C_CONTROL_PROTOCOL_EN.md)

## Repository Structure

```text
.
├── code
│   ├── Unit-TrackBall        # Application firmware
│   └── Unit-TrackBall-IAP    # IAP bootloader
├── firmware
│   └── 2026-11-11-0x01       # Released binary, hex and C header images
├── protocol                  # I2C control protocol document
├── .clang-format
├── .github
└── LICENSE
```

## Firmware Projects

| Project | Path | Description |
| --- | --- | --- |
| Unit-TrackBall | `code/Unit-TrackBall` | Application firmware, linked at `0x08001800` |
| Unit-TrackBall-IAP | `code/Unit-TrackBall-IAP` | IAP bootloader, linked at `0x08000000` |

## Hardware & Firmware Notes

- MCU: STM32G030F6P6 (Cortex-M0+, 32 KB flash / 8 KB RAM)
- Trackball sensor: MOT6010, connected to the internal I2C bus at 7-bit address `0x74`
- Host interface: I2C slave, default 7-bit address `0x3F`, configurable `0x08 ~ 0x77`, up to 400 kHz
- Flash layout: bootloader `0x08000000`–`0x080017FF`, application `0x08001800`–`0x080077FF`, configuration page `0x08007800`–`0x08007FFF`
- Firmware version: `USER_I2C_SW_VERSION` in `code/Unit-TrackBall/Core/User/i2c/user_i2c_reg.h`

## Build

Requirements: CMake, Ninja and the `arm-none-eabi-gcc` toolchain.

```sh
(cd code/Unit-TrackBall-IAP && cmake --preset Release && cmake --build --preset Release)
(cd code/Unit-TrackBall && cmake --preset Release && cmake --build --preset Release)
```

Both Release presets build with `-O1 -g0`.

## Firmware Images

`firmware/2026-11-11-0x01/` contains the packaged images for firmware version `0x01`:

- `U243-Unit-TrackBall-STM32G030F6P6-TAO-0x01-20261111_0x8000000.bin` / `.hex` — combined bootloader and application image starting at `0x08000000`
- `bootloader.h`, `app_without_flash.h`, `app_with_flash.h` — C header images used by factory and upgrade tooling

## Formatting

C/C++ sources follow the repository [.clang-format](.clang-format) and are checked by [clang-format-check.yml](.github/workflows/clang-format-check.yml) with clang-format 18. Generated code and the firmware images under `firmware/` are excluded from the check.

## License

- [M5Unit-TrackBall - MIT](LICENSE)
