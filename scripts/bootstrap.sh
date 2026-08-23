#!/usr/bin/env bash
set -euo pipefail
PROJECT_ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
python3 -m pip install --upgrade -r "$PROJECT_ROOT/requirements-platformio.txt"
python3 -m platformio --version
