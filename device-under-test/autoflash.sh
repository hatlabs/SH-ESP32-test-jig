#!/usr/bin/env bash

set -euo pipefail
shopt -s inherit_errexit

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# Frozen prebuilt images. Flashing never rebuilds, so the test binary stays
# byte-identical regardless of the local PlatformIO / toolchain version.
FW="$SCRIPT_DIR/firmware"

# Flash with the esptool that ships inside PlatformIO, so the host needs no
# separately installed esptool. PlatformIO's own interpreter (which carries
# esptool's dependencies) comes from the `pio` launcher shebang; esptool itself
# lives in the core packages directory. Only version-independent esptool
# arguments are used, so this works across the esptool releases that different
# platform versions bundle.
PIO_BIN="$(command -v pio || true)"
if [ -z "$PIO_BIN" ]; then
    echo "PlatformIO (pio) not found on PATH." >&2
    exit 1
fi
CORE_DIR="${PLATFORMIO_CORE_DIR:-$HOME/.platformio}"
ESPTOOL="$CORE_DIR/packages/tool-esptoolpy/esptool.py"
PYEXE="$(head -n1 "$PIO_BIN" | sed 's/^#! *//')"
if [ ! -x "$PYEXE" ]; then
    PYEXE="$CORE_DIR/penv/bin/python"
fi
if [ ! -x "$PYEXE" ] || [ ! -f "$ESPTOOL" ]; then
    echo "Could not locate PlatformIO's esptool." >&2
    echo "  interpreter: $PYEXE" >&2
    echo "  esptool:     $ESPTOOL" >&2
    exit 1
fi

SERIAL_DEV=$(ls /dev/tty.usbserial-* | head -n 1)

while true; do
    echo "Insert device."
    if [ -c $SERIAL_DEV ]; then
        "$PYEXE" "$ESPTOOL" --chip esp32 --port "$SERIAL_DEV" --baud 2000000 \
            write_flash --compress --flash_size detect \
            0x1000 "$FW/bootloader.bin" \
            0x8000 "$FW/partitions.bin" \
            0xe000 "$FW/boot_app0.bin" \
            0x10000 "$FW/firmware.bin"
        echo "Done. Remove device."
        echo -e '\a'
        while [ -c $SERIAL_DEV ] ; do
            sleep 1
        done
    fi
    sleep 1
done
