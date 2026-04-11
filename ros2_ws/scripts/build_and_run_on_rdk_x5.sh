#!/usr/bin/env bash
set -euo pipefail

RDK_USER="${RDK_USER:-sunrise}"
RDK_HOST="${RDK_HOST:?Please set RDK_HOST, e.g. export RDK_HOST=192.168.1.10}"
RDK_PORT="${RDK_PORT:-22}"
REMOTE_WS="${REMOTE_WS:-/home/sunrise/rm_ws}"
REMOTE_SRC_DIR="${REMOTE_SRC_DIR:-${REMOTE_WS}/src}"
REMOTE_SCRIPT_DIR="${REMOTE_SCRIPT_DIR:-${REMOTE_SRC_DIR}/scripts}"
SERIAL_PORT="${SERIAL_PORT:-/dev/ttyS1}"
ENEMY_PREFIX="${ENEMY_PREFIX:-}"

read -r -d '' REMOTE_CMD <<EOF || true
set -e
cd "${REMOTE_WS}"
source /opt/tros/humble/setup.bash
SERIAL_PORT="${SERIAL_PORT}" ENEMY_PREFIX="${ENEMY_PREFIX}" REMOTE_WS="${REMOTE_WS}" REMOTE_SRC_DIR="${REMOTE_SRC_DIR}" REMOTE_SCRIPT_DIR="${REMOTE_SCRIPT_DIR}" bash "${REMOTE_SCRIPT_DIR}/clean_build_and_start_on_rdk.sh"
EOF

ssh -t -p "${RDK_PORT}" "${RDK_USER}@${RDK_HOST}" "${REMOTE_CMD}"
