#!/usr/bin/env bash
set -euo pipefail
PORT=${1:?Usage: ./scripts/flash.sh /dev/ttyUSB0 [environment]}
ENVIRONMENT=${2:-crowpanel_idf5}
ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
cd "$ROOT"
python3 -m platformio run -e "$ENVIRONMENT" -t upload --upload-port "$PORT"
