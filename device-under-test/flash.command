#!/usr/bin/env bash
#
# Double-clickable launcher. Finder runs this in Terminal, so nothing
# needs to be typed: insert a device, wait for the beep, swap the next one in.

cd "$(dirname "$0")" || exit 1
exec ./autoflash.sh
