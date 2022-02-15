#!/usr/bin/env bash

set -euo pipefail
shopt -s inherit_errexit

SERIAL_DEV=/dev/tty.usbserial-310

while true; do
    echo "Insert device."
    if [ -c $SERIAL_DEV ]; then
        pio run -e esp32dev -t upload
        echo "Done. Remove device."
        while [ -c $SERIAL_DEV ] ; do
            sleep 1
        done
    fi
    sleep 1
done
