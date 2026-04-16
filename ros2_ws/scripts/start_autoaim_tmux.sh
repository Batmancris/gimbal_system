#!/usr/bin/env bash
set -eo pipefail

REMOTE_WS="${REMOTE_WS:-/home/sunrise/rm_ws}"
REMOTE_SRC_DIR="${REMOTE_SRC_DIR:-${REMOTE_WS}/src}"
REMOTE_SCRIPT_DIR="${REMOTE_SCRIPT_DIR:-${REMOTE_SRC_DIR}/scripts}"
STARTUP_DELAY_SEC="${STARTUP_DELAY_SEC:-0}"
DETECTOR_DELAY_SEC="${DETECTOR_DELAY_SEC:-0}"
CAMERA_READY_TIMEOUT_SEC="${CAMERA_READY_TIMEOUT_SEC:-8}"
TMUX_SOCKET="${TMUX_SOCKET:-autoaim}"
DETECTOR_TYPE="${DETECTOR_TYPE:-armor}"
DETECTOR_TOPIC="${DETECTOR_TOPIC:-/dnn_node_sample}"
VEHICLE_MODEL_PATH="${VEHICLE_MODEL_PATH:-}"

tmux_cmd() {
  tmux -L "${TMUX_SOCKET}" "$@"
}

if [ "${STARTUP_DELAY_SEC}" -gt 0 ] 2>/dev/null; then
  sleep "${STARTUP_DELAY_SEC}"
fi

cd "${REMOTE_WS}"
set +u
source /opt/tros/humble/setup.bash
source install/setup.bash || true

pkill -f "rm_armor_detection rm_armor_detection" 2>/dev/null || true
pkill -f hik_camera_node 2>/dev/null || true

tmux_cmd kill-session -t rm_det 2>/dev/null || true
tmux_cmd kill-session -t hik_cam 2>/dev/null || true
tmux_cmd kill-session -t rm_vis 2>/dev/null || true

tmux_cmd new-session -d -s hik_cam "env REMOTE_WS='${REMOTE_WS}' REMOTE_SRC_DIR='${REMOTE_SRC_DIR}' REMOTE_SCRIPT_DIR='${REMOTE_SCRIPT_DIR}' bash '${REMOTE_SCRIPT_DIR}/run_hik_cam_loop.sh'"

camera_ready_deadline=$((SECONDS + CAMERA_READY_TIMEOUT_SEC))
while [ "${SECONDS}" -lt "${camera_ready_deadline}" ]; do
  if ros2 topic info /image_raw 2>/dev/null | grep -q "Publisher count: 1"; then
    break
  fi
  sleep 0.5
done

if [ "${DETECTOR_DELAY_SEC}" -gt 0 ] 2>/dev/null; then
  sleep "${DETECTOR_DELAY_SEC}"
fi

tmux_cmd new-session -d -s rm_det "env REMOTE_WS='${REMOTE_WS}' REMOTE_SRC_DIR='${REMOTE_SRC_DIR}' REMOTE_SCRIPT_DIR='${REMOTE_SCRIPT_DIR}' DETECTOR_TYPE='${DETECTOR_TYPE}' DETECTOR_TOPIC='${DETECTOR_TOPIC}' VEHICLE_MODEL_PATH='${VEHICLE_MODEL_PATH}' bash '${REMOTE_SCRIPT_DIR}/run_rm_det_loop.sh'"
tmux_cmd new-session -d -s rm_vis "env REMOTE_WS='${REMOTE_WS}' REMOTE_SRC_DIR='${REMOTE_SRC_DIR}' REMOTE_SCRIPT_DIR='${REMOTE_SCRIPT_DIR}' DETECTOR_TOPIC='${DETECTOR_TOPIC}' DISPLAY_VALUE='${DISPLAY_VALUE:-:0}' XAUTHORITY_VALUE='${XAUTHORITY_VALUE:-/home/sunrise/.Xauthority}' XDG_RUNTIME_DIR_VALUE='${XDG_RUNTIME_DIR_VALUE:-/run/user/1000}' VIS_DELAY_SEC='${VIS_DELAY_SEC:-3}' bash '${REMOTE_SCRIPT_DIR}/run_rm_vis_loop.sh'"

echo "tmux sessions started:"
tmux_cmd ls || true
