#!/usr/bin/env bash

set -euo pipefail
shopt -s inherit_errexit

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# Frozen prebuilt images. Flashing never rebuilds, so the test binary stays
# byte-identical regardless of the local PlatformIO / toolchain version.
FW="$SCRIPT_DIR/firmware"

SERIAL_DEV=$(ls /dev/tty.usbserial-* | head -n 1)

while true; do
    echo "Insert device."
    if [ -c $SERIAL_DEV ]; then
        esptool --chip esp32 --port "$SERIAL_DEV" --baud 2000000 \
            --before default-reset --after hard-reset \
            write-flash --compress \
            --flash-mode dio --flash-freq 40m --flash-size detect \
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
