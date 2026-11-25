#!/usr/bin/env bash

set -euo pipefail
shopt -s inherit_errexit

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# Frozen prebuilt images. Flashing never rebuilds, so the test binary stays
# byte-identical regardless of the local PlatformIO / toolchain version.
FW="$SCRIPT_DIR/firmware"

# Flash with the esptool that ships inside PlatformIO, so the host needs no
# separately installed esptool. Its dependencies live in whichever interpreter
# PlatformIO installed them into: newer espressif32 platforms (esptool 5.x,
# which needs rich_click) use $CORE_DIR/penv, while older ones rely on the
# interpreter running PlatformIO itself (the `pio` launcher shebang). Use the
# first candidate that can actually run esptool. Only version-independent
# esptool arguments are used, so this works across bundled esptool releases.
PIO_BIN="$(command -v pio || true)"
if [ -z "$PIO_BIN" ]; then
    echo "PlatformIO (pio) not found on PATH." >&2
    exit 1
fi
CORE_DIR="${PLATFORMIO_CORE_DIR:-$HOME/.platformio}"
ESPTOOL="$CORE_DIR/packages/tool-esptoolpy/esptool.py"
if [ ! -f "$ESPTOOL" ]; then
    echo "PlatformIO's esptool not found at $ESPTOOL" >&2
    exit 1
fi
CANDIDATES=("$CORE_DIR/penv/bin/python" "$(head -n1 "$PIO_BIN" | sed 's/^#! *//')")
PYEXE=""
for candidate in "${CANDIDATES[@]}"; do
    if [ -x "$candidate" ] && "$candidate" "$ESPTOOL" version >/dev/null 2>&1; then
        PYEXE="$candidate"
        break
    fi
done
if [ -z "$PYEXE" ]; then
    echo "No PlatformIO interpreter can run $ESPTOOL; tried:" >&2
    for candidate in "${CANDIDATES[@]}"; do
        if [ -x "$candidate" ]; then
            reason="$("$candidate" "$ESPTOOL" version 2>&1 | tail -n1 || true)"
        else
            reason="not an executable"
        fi
        echo "  $candidate: $reason" >&2
    done
    exit 1
fi

# At 2000000 baud the board stops answering once esptool 5.1.0 switches to
# that rate ("The chip stopped responding"); 921600 works.
FLASH_BAUD=921600

while true; do
    echo "Insert device."
    SERIAL_DEV=$(ls /dev/tty.usbserial-* 2>/dev/null | head -n 1 || true)
    if [ -n "$SERIAL_DEV" ] && [ -c "$SERIAL_DEV" ]; then
        "$PYEXE" "$ESPTOOL" --chip esp32 --port "$SERIAL_DEV" --baud "$FLASH_BAUD" \
            write_flash --compress --flash_size detect \
            0x1000 "$FW/bootloader.bin" \
            0x8000 "$FW/partitions.bin" \
            0xe000 "$FW/boot_app0.bin" \
            0x10000 "$FW/firmware.bin"
        echo "Done. Remove device."
        echo -e '\a'
        while [ -c "$SERIAL_DEV" ] ; do
            sleep 1
        done
    fi
    sleep 1
done
