#!/usr/bin/env bash
# copy_output.sh
# Collect UF2 files from build/… into ./output/ with friendly names.

set -euo pipefail

# Map "build sub-dir" → "desired filename"
declare -A FW_MAP=(
  [left]="left.uf2"
  [right]="right.uf2"
  [dongle]="dongle.uf2"
  [reset]="reset.uf2"
)

DEST_DIR="$(dirname "$0")/output"
mkdir -p "$DEST_DIR"

for key in "${!FW_MAP[@]}"; do
  SRC="build/${key}/zephyr/zmk.uf2"
  DST="${DEST_DIR}/${FW_MAP[$key]}"
  if [[ -f "$SRC" ]]; then
    cp -f "$SRC" "$DST"
    echo "Copied $SRC -> $DST"
  else
    echo "Skipped $key  (source not found: $SRC)" >&2
  fi
done

echo -e "\nAll done. Firmware files are in: $DEST_DIR"
