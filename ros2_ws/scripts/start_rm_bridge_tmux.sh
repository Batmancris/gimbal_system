#!/usr/bin/env bash
set -eo pipefail

REMOTE_WS="${REMOTE_WS:-/home/sunrise/rm_ws}"
if [ -n "${REMOTE_SRC_DIR:-}" ]; then
  REMOTE_SRC_DIR="${REMOTE_SRC_DIR}"
elif [ -d "${REMOTE_WS}/src/ros2_ws/scripts" ]; then
  REMOTE_SRC_DIR="${REMOTE_WS}/src/ros2_ws"
else
  REMOTE_SRC_DIR="${REMOTE_WS}/src"
fi
if [ -n "${REMOTE_SCRIPT_DIR:-}" ]; then
  REMOTE_SCRIPT_DIR="${REMOTE_SCRIPT_DIR}"
else
  REMOTE_SCRIPT_DIR="${REMOTE_SRC_DIR}/scripts"
fi
TMUX_SOCKET="${TMUX_SOCKET:-bridge}"

tmux_cmd() {
  tmux -L "${TMUX_SOCKET}" "$@"
}

pkill -f rm_gimbal_bridge_node 2>/dev/null || true
tmux_cmd kill-session -t rm_bridge 2>/dev/null || true

tmux_cmd new-session -d -s rm_bridge "env REMOTE_WS='${REMOTE_WS}' REMOTE_SRC_DIR='${REMOTE_SRC_DIR}' REMOTE_SCRIPT_DIR='${REMOTE_SCRIPT_DIR}' SERIAL_PORT='${SERIAL_PORT:-/dev/serial/by-id/usb-Batmancris_Gimbal_Control_CDC_3162376B3439-if00}' ENEMY_PREFIX='${ENEMY_PREFIX:-}' BRIDGE_DELAY_SEC='${BRIDGE_DELAY_SEC:-0}' WAIT_FOR_SERIAL_SEC='${WAIT_FOR_SERIAL_SEC:-15}' BRIDGE_REQUIRE_VISION_ENABLED='${BRIDGE_REQUIRE_VISION_ENABLED:-false}' DETECTOR_READY_TIMEOUT_SEC='${DETECTOR_READY_TIMEOUT_SEC:-10}' bash '${REMOTE_SCRIPT_DIR}/run_rm_bridge_loop.sh'"

echo "tmux sessions started:"
tmux_cmd ls || true
