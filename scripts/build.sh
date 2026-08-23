#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
cd "$ROOT"
ENVIRONMENT=${1:-crowpanel_idf5}
python3 -m platformio run -e "$ENVIRONMENT"
python3 tools/export_firmware.py --env "$ENVIRONMENT"
