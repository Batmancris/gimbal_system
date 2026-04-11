#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
PYTHON_EXE="$(bash "$SCRIPT_DIR/bootstrap_env.sh")"

cd "$PROJECT_DIR"
"$PYTHON_EXE" train_yolov8.py --mode check "$@"
