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
REMOTE_SCRIPT_DIR="${REMOTE_SCRIPT_DIR:-${REMOTE_SRC_DIR}/scripts}"
TMUX_SOCKET="${TMUX_SOCKET:-autoaim}"
DETECTOR_TOPIC="${DETECTOR_TOPIC:-/vehicle_detection/targets}"
DISPLAY_VALUE="${DISPLAY_VALUE:-:0.0}"
XAUTHORITY_VALUE="${XAUTHORITY_VALUE:-/home/sunrise/.Xauthority}"
XDG_RUNTIME_DIR_VALUE="${XDG_RUNTIME_DIR_VALUE:-/run/user/1000}"
VIS_DELAY_SEC="${VIS_DELAY_SEC:-0}"
VIS_MAX_FPS="${VIS_MAX_FPS:-12.0}"
VIS_SCALE="${VIS_SCALE:-0.75}"
VIS_DEBUG_TOPIC="${VIS_DEBUG_TOPIC:-/vehicle_detection/debug_text}"
VIS_SHOW_DEBUG_TEXT="${VIS_SHOW_DEBUG_TEXT:-true}"

tmux -L "${TMUX_SOCKET}" kill-session -t rm_vis 2>/dev/null || true
tmux -L "${TMUX_SOCKET}" new-session -d -s rm_vis \
  "env REMOTE_WS='${REMOTE_WS}' REMOTE_SRC_DIR='${REMOTE_SRC_DIR}' REMOTE_SCRIPT_DIR='${REMOTE_SCRIPT_DIR}' DETECTOR_TOPIC='${DETECTOR_TOPIC}' DISPLAY_VALUE='${DISPLAY_VALUE}' XAUTHORITY_VALUE='${XAUTHORITY_VALUE}' XDG_RUNTIME_DIR_VALUE='${XDG_RUNTIME_DIR_VALUE}' VIS_DELAY_SEC='${VIS_DELAY_SEC}' VIS_MAX_FPS='${VIS_MAX_FPS}' VIS_SCALE='${VIS_SCALE}' VIS_DEBUG_TOPIC='${VIS_DEBUG_TOPIC}' VIS_SHOW_DEBUG_TEXT='${VIS_SHOW_DEBUG_TEXT}' bash '${REMOTE_SCRIPT_DIR}/run_rm_vis_loop.sh'"

echo "rm_vis started in tmux socket ${TMUX_SOCKET}, session rm_vis"
