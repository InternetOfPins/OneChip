#!/usr/bin/env bash
# record_size.sh — build blink, extract binary sizes, append to results/size_history.csv
# Usage: ./record_size.sh "description of what changed" [board=uno]

set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CSV="$SCRIPT_DIR/results/size_history.csv"
PIO="${PIO:-$HOME/.platformio/penv/bin/pio}"

DESC="${1:-}"
BOARD="${2:-uno}"
if [ -z "$DESC" ]; then
  echo "Usage: $0 \"description\" [board=uno]"
  exit 1
fi

cd "$SCRIPT_DIR/.."
OUTPUT=$("$PIO" run -e "$BOARD" 2>&1)
echo "$OUTPUT" | tail -6

FLASH_BYTES=$(echo "$OUTPUT" | grep "Flash:" | grep -oP 'used \K[0-9]+')
RAM_BYTES=$(echo   "$OUTPUT" | grep "RAM:"   | grep -oP 'used \K[0-9]+')
GCC_VER=$(avr-gcc --version 2>/dev/null | head -1 | grep -oP '\d+\.\d+\.\d+' | head -1 || echo "?")
DATE=$(date +%Y-%m-%d)

echo "${DATE},${BOARD},${FLASH_BYTES},${RAM_BYTES},${GCC_VER},${DESC}" >> "$CSV"
echo ""
echo "Recorded: flash=${FLASH_BYTES} B  ram=${RAM_BYTES} B  gcc=${GCC_VER}  board=${BOARD}"
echo "  $DESC"
