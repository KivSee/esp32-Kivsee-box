#!/bin/bash

# OTA firmware upload script for the RFID boxes
# Usage: ./ota_upload.sh [box_numbers...]
#   No arguments: uploads to all boxes (1-5)
#   With arguments: uploads only to the specified boxes
#   Examples:
#     ./ota_upload.sh           # all boxes 1-5
#     ./ota_upload.sh 3         # only box3
#     ./ota_upload.sh 1 4       # only box1 and box4
#
# Unlike the rings, a box's name is compiled in (BOX_NAME: MQTT topics, WiFi/OTA
# hostname), so each box gets its own build. BOX_NAME is passed as a build flag,
# overriding the default in include/secrets.h.

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PIO_ENV="esp32dev"
PIO="$HOME/.platformio/penv/Scripts/pio"

# Devices: use arguments if provided, otherwise all boxes 1-5
if [ $# -gt 0 ]; then
    DEVICES=("$@")
else
    DEVICES=(1 2 3 4 5)
fi

for num in "${DEVICES[@]}"; do
    if ! [[ "$num" =~ ^[1-9][0-9]*$ ]]; then
        echo "ERROR: '$num' is not a box number (expected e.g. 1 3 5)"
        exit 1
    fi
done

cd "$SCRIPT_DIR"

# Build and upload each box (an upload to boxN.local goes over the network: espota)
succeeded=()
failed=()

for num in "${DEVICES[@]}"; do
    name="box${num}"
    host="${name}.local"
    echo "========================================"
    echo "Building and uploading $name to $host ..."
    echo "========================================"

    if PLATFORMIO_BUILD_FLAGS="-DBOX_NAME=\\\"${name}\\\"" \
        "$PIO" run -e "$PIO_ENV" -t upload --upload-port "$host"; then
        echo "OK: $name"
        succeeded+=("$name")
    else
        echo "FAILED: $name"
        failed+=("$name")
    fi
    echo ""
done

# Summary
echo "========================================"
echo "UPLOAD SUMMARY"
echo "========================================"
echo "Succeeded (${#succeeded[@]}/${#DEVICES[@]}):"
for h in "${succeeded[@]}"; do
    echo "  + $h"
done

if [ ${#failed[@]} -gt 0 ]; then
    echo ""
    echo "Failed (${#failed[@]}/${#DEVICES[@]}):"
    for h in "${failed[@]}"; do
        echo "  - $h"
    done
    exit 1
fi

echo ""
echo "All boxes updated successfully!"
