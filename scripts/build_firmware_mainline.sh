#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/tianaim_paths.sh"

make -C "${TIANAIM_FIRMWARE_DIR}"
