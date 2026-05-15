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
STARTUP_DELAY_SEC="${STARTUP_DELAY_SEC:-0}"
DETECTOR_DELAY_SEC="${DETECTOR_DELAY_SEC:-0}"
CAMERA_READY_TIMEOUT_SEC="${CAMERA_READY_TIMEOUT_SEC:-8}"
TMUX_SOCKET="${TMUX_SOCKET:-autoaim}"
DETECTOR_TYPE="${DETECTOR_TYPE:-bear}"
DETECTOR_TOPIC="${DETECTOR_TOPIC:-/bear_detection/targets}"
VEHICLE_MODEL_PATH="${VEHICLE_MODEL_PATH:-}"
ENABLE_VISUALIZER="${ENABLE_VISUALIZER:-false}"
VIS_MAX_FPS="${VIS_MAX_FPS:-24.0}"
VIS_SCALE="${VIS_SCALE:-1.0}"
VIS_DEBUG_TOPIC="${VIS_DEBUG_TOPIC:-/bear_detection/debug_text}"
VIS_SHOW_DEBUG_TEXT="${VIS_SHOW_DEBUG_TEXT:-false}"
VIS_FULLSCREEN="${VIS_FULLSCREEN:-false}"
VIS_KEEP_ASPECT_RATIO="${VIS_KEEP_ASPECT_RATIO:-true}"
VIS_WINDOW_WIDTH="${VIS_WINDOW_WIDTH:-1920}"
VIS_WINDOW_HEIGHT="${VIS_WINDOW_HEIGHT:-1080}"
VEHICLE_PUBLISH_DEBUG_TEXT="${VEHICLE_PUBLISH_DEBUG_TEXT:-false}"
VEHICLE_LOG_LEVEL="${VEHICLE_LOG_LEVEL:-warn}"
BEAR_LOG_LEVEL="${BEAR_LOG_LEVEL:-warn}"
BEAR_SCORE_THRESHOLD="${BEAR_SCORE_THRESHOLD:-0.71}"
BEAR_STABLE_REQUIRED_HITS="${BEAR_STABLE_REQUIRED_HITS:-2}"
BEAR_STABLE_MATCH_RADIUS_PX="${BEAR_STABLE_MATCH_RADIUS_PX:-140.0}"
BEAR_STABLE_MAX_TRACK_AGE_MS="${BEAR_STABLE_MAX_TRACK_AGE_MS:-200}"
BEAR_PUBLISH_DEBUG_TEXT="${BEAR_PUBLISH_DEBUG_TEXT:-false}"
BEAR_LOG_DETECTIONS="${BEAR_LOG_DETECTIONS:-false}"

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

pkill -f "[r]m_armor_detection rm_armor_detection" 2>/dev/null || true
pkill -f "[r]m_vehicle_detection rm_vehicle_detection_node" 2>/dev/null || true
pkill -f "[r]m_bear_detection rm_bear_detection_node" 2>/dev/null || true
pkill -f "[h]ik_camera_node" 2>/dev/null || true

tmux_cmd kill-session -t rm_det 2>/dev/null || true
tmux_cmd kill-session -t hik_cam 2>/dev/null || true
tmux_cmd kill-session -t rm_vis 2>/dev/null || true

tmux_cmd new-session -d -s hik_cam "env REMOTE_WS='${REMOTE_WS}' REMOTE_SRC_DIR='${REMOTE_SRC_DIR}' REMOTE_SCRIPT_DIR='${REMOTE_SCRIPT_DIR}' bash '${REMOTE_SCRIPT_DIR}/run_hik_cam_loop.sh'"

camera_ready_deadline=$((SECONDS + CAMERA_READY_TIMEOUT_SEC))
while [ "${SECONDS}" -lt "${camera_ready_deadline}" ]; do
  if timeout 2 ros2 topic info /hbmem_img 2>/dev/null | grep -q "Publisher count: 1"; then
    break
  fi
  sleep 0.5
done

if [ "${DETECTOR_DELAY_SEC}" -gt 0 ] 2>/dev/null; then
  sleep "${DETECTOR_DELAY_SEC}"
fi

tmux_cmd new-session -d -s rm_det "env REMOTE_WS='${REMOTE_WS}' REMOTE_SRC_DIR='${REMOTE_SRC_DIR}' REMOTE_SCRIPT_DIR='${REMOTE_SCRIPT_DIR}' DETECTOR_TYPE='${DETECTOR_TYPE}' DETECTOR_TOPIC='${DETECTOR_TOPIC}' VEHICLE_MODEL_PATH='${VEHICLE_MODEL_PATH}' VEHICLE_PUBLISH_DEBUG_TEXT='${VEHICLE_PUBLISH_DEBUG_TEXT}' VEHICLE_LOG_LEVEL='${VEHICLE_LOG_LEVEL}' BEAR_LOG_LEVEL='${BEAR_LOG_LEVEL}' BEAR_SCORE_THRESHOLD='${BEAR_SCORE_THRESHOLD}' BEAR_STABLE_REQUIRED_HITS='${BEAR_STABLE_REQUIRED_HITS}' BEAR_STABLE_MATCH_RADIUS_PX='${BEAR_STABLE_MATCH_RADIUS_PX}' BEAR_STABLE_MAX_TRACK_AGE_MS='${BEAR_STABLE_MAX_TRACK_AGE_MS}' BEAR_PUBLISH_DEBUG_TEXT='${BEAR_PUBLISH_DEBUG_TEXT}' BEAR_LOG_DETECTIONS='${BEAR_LOG_DETECTIONS}' bash '${REMOTE_SCRIPT_DIR}/run_rm_det_loop.sh'"

if [ "${ENABLE_VISUALIZER}" = "true" ]; then
  tmux_cmd new-session -d -s rm_vis "env REMOTE_WS='${REMOTE_WS}' REMOTE_SRC_DIR='${REMOTE_SRC_DIR}' REMOTE_SCRIPT_DIR='${REMOTE_SCRIPT_DIR}' DETECTOR_TOPIC='${DETECTOR_TOPIC}' DISPLAY_VALUE='${DISPLAY_VALUE:-:0.0}' XAUTHORITY_VALUE='${XAUTHORITY_VALUE:-/home/sunrise/.Xauthority}' XDG_RUNTIME_DIR_VALUE='${XDG_RUNTIME_DIR_VALUE:-/run/user/1000}' VIS_DELAY_SEC='${VIS_DELAY_SEC:-3}' VIS_MAX_FPS='${VIS_MAX_FPS}' VIS_SCALE='${VIS_SCALE}' VIS_DEBUG_TOPIC='${VIS_DEBUG_TOPIC}' VIS_SHOW_DEBUG_TEXT='${VIS_SHOW_DEBUG_TEXT}' VIS_FULLSCREEN='${VIS_FULLSCREEN}' VIS_KEEP_ASPECT_RATIO='${VIS_KEEP_ASPECT_RATIO}' VIS_WINDOW_WIDTH='${VIS_WINDOW_WIDTH}' VIS_WINDOW_HEIGHT='${VIS_WINDOW_HEIGHT}' bash '${REMOTE_SCRIPT_DIR}/run_rm_vis_loop.sh'"
fi

echo "tmux sessions started:"
tmux_cmd ls || true
