#!/usr/bin/env bash
set -euo pipefail
PORT=${1:?Usage: ./scripts/monitor.sh /dev/ttyUSB0}
ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
cd "$ROOT"
python3 -m platformio device monitor --port "$PORT" --baud 115200 --filter time --filter colorize --filter esp32_exception_decoder
