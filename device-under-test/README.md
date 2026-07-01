# SH-ESP32 test jig DUT flashing

Flashes the frozen SH-ESP32 test-jig DUT firmware onto ESP32 devices, one after
another, for end-of-line testing. The binaries in `firmware/` are fixed and are
**never rebuilt** — every device gets identical bytes regardless of the toolchain
on this machine.

## One-time setup

`autoflash.sh` flashes with the esptool that ships inside PlatformIO's core
packages (`~/.platformio`), so no separately installed esptool is needed. With
PlatformIO Core installed, fetch just the esptool package globally — no project
checkout required:

```bash
pio pkg install -g -t platformio/tool-esptoolpy
```

Set `PLATFORMIO_CORE_DIR` if PlatformIO's core lives somewhere other than
`~/.platformio`.

## Flashing

Double-click `flash.command` (opens a Terminal window), or run `./autoflash.sh`.

Then, per device:

1. Plug the device in.
2. Wait for the beep and `Done. Remove device.`
3. Unplug it and plug in the next one.

Press Ctrl-C to stop.

## Contents

| Path | Purpose |
|------|---------|
| `autoflash.sh` | Flash loop (drives PlatformIO's esptool) |
| `flash.command` | Double-click launcher for `autoflash.sh` |
| `firmware/` | Frozen flash images: bootloader, partitions, boot_app0, firmware |

## Refreshing the firmware

Building is a separate, deliberate step. The DUT source builds against
`espressif32` (arduino-esp32 3.x), ReactESP 3.x, and `NMEA2000_twai`. To cut a
new frozen binary, rebuild the PlatformIO project (`pio run -e esp32dev`) and copy
the resulting `bootloader.bin`, `partitions.bin`, and `firmware.bin` from
`.pio/build/esp32dev/` into `firmware/` (plus `boot_app0.bin` from the Arduino
framework package). Then push the updated bundle to the Manufacturing shared
drive with `./run sync-drive`.
