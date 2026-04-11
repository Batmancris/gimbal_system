#!/usr/bin/env bash
set -eo pipefail

REMOTE_WS="${REMOTE_WS:-/home/sunrise/rm_ws}"
REMOTE_SRC_DIR="${REMOTE_SRC_DIR:-${REMOTE_WS}/src}"
REMOTE_SCRIPT_DIR="${REMOTE_SCRIPT_DIR:-${REMOTE_SRC_DIR}/scripts}"
SERIAL_PORT="${SERIAL_PORT:-/dev/ttyS1}"
ENEMY_PREFIX="${ENEMY_PREFIX:-}"
PACKAGES="${PACKAGES:-hik_camera rm_armor_detection rm_gimbal_bridge}"

cd "${REMOTE_WS}"
source /opt/tros/humble/setup.bash

pkill -f "rm_armor_detection rm_armor_detection" 2>/dev/null || true
pkill -f rm_gimbal_bridge_node 2>/dev/null || true
pkill -f hik_camera_node 2>/dev/null || true

tmux kill-session -t rm_det 2>/dev/null || true
tmux kill-session -t rm_bridge 2>/dev/null || true
tmux kill-session -t hik_cam 2>/dev/null || true

rm -rf build install log
mkdir -p "${REMOTE_SRC_DIR}"

colcon build --packages-select ${PACKAGES} --event-handlers console_direct+

source install/setup.bash
SERIAL_PORT="${SERIAL_PORT}" ENEMY_PREFIX="${ENEMY_PREFIX}" REMOTE_WS="${REMOTE_WS}" REMOTE_SRC_DIR="${REMOTE_SRC_DIR}" REMOTE_SCRIPT_DIR="${REMOTE_SCRIPT_DIR}" \
  bash "${REMOTE_SCRIPT_DIR}/start_autoaim_tmux.sh"
