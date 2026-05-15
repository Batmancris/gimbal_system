#!/usr/bin/env bash
set -eo pipefail

# Lightweight headless follow: camera + bear detection + gimbal bridge only.
# No visualizer, no image_raw, no debug logs.

REMOTE_WS="${REMOTE_WS:-/home/sunrise/rm_ws}"
if [ -n "${REMOTE_SRC_DIR:-}" ]; then
  REMOTE_SRC_DIR="${REMOTE_SRC_DIR}"
elif [ -d "${REMOTE_WS}/src/ros2_ws/scripts" ]; then
  REMOTE_SRC_DIR="${REMOTE_WS}/src/ros2_ws"
else
  REMOTE_SRC_DIR="${REMOTE_WS}/src"
fi
REMOTE_SCRIPT_DIR="${REMOTE_SCRIPT_DIR:-${REMOTE_SRC_DIR}/scripts}"

SERIAL_PORT="${SERIAL_PORT:-/dev/serial/by-id/usb-Batmancris_Gimbal_Control_CDC_3162376B3439-if00}"
ALLOWED_TARGET_TYPES="${ALLOWED_TARGET_TYPES:-bear}"
DETECTOR_TOPIC="${DETECTOR_TOPIC:-/bear_detection/targets}"
BEAR_LOG_LEVEL="${BEAR_LOG_LEVEL:-warn}"
BEAR_SCORE_THRESHOLD="${BEAR_SCORE_THRESHOLD:-0.71}"
BEAR_STABLE_REQUIRED_HITS="${BEAR_STABLE_REQUIRED_HITS:-2}"
BEAR_STABLE_MATCH_RADIUS_PX="${BEAR_STABLE_MATCH_RADIUS_PX:-140.0}"
BEAR_STABLE_MAX_TRACK_AGE_MS="${BEAR_STABLE_MAX_TRACK_AGE_MS:-200}"
BRIDGE_MIN_CONFIDENCE="${BRIDGE_MIN_CONFIDENCE:-0.71}"
FOLLOW_PROFILE="${FOLLOW_PROFILE:-stable}"

# NOTE: Do NOT set FOLLOW_SEND_RATE_HZ here.
# It is controlled by the profile in run_rm_bridge_loop.sh.

TMUX_SOCKET="${TMUX_SOCKET:-autoaim}"

tmux_cmd() {
  tmux -L "${TMUX_SOCKET}" "$@"
}

cd "${REMOTE_WS}"
set +u
source /opt/tros/humble/setup.bash
source install/setup.bash || true

# --- kill stale processes ---
pkill -f rm_autoaim_visualizer 2>/dev/null || true
pkill -f rm_vis 2>/dev/null || true
pkill -f 'ros2 topic echo' 2>/dev/null || true
pkill -f 'ros2 topic hz' 2>/dev/null || true
pkill -f profile_targets 2>/dev/null || true
pkill -f profile_topic_jitter 2>/dev/null || true
pkill -f baseline_metrics 2>/dev/null || true
pkill -f verify_target_center 2>/dev/null || true
pkill -f "[r]m_armor_detection rm_armor_detection" 2>/dev/null || true
pkill -f "[r]m_vehicle_detection rm_vehicle_detection_node" 2>/dev/null || true
pkill -f "[r]m_bear_detection rm_bear_detection_node" 2>/dev/null || true
pkill -f "[h]ik_camera_node" 2>/dev/null || true
pkill -f "[r]m_gimbal_bridge_node" 2>/dev/null || true

tmux_cmd kill-session -t hik_cam 2>/dev/null || true
tmux_cmd kill-session -t rm_det 2>/dev/null || true
tmux_cmd kill-session -t rm_bridge 2>/dev/null || true

# --- start camera ---
tmux_cmd new-session -d -s hik_cam \
  "env REMOTE_WS='${REMOTE_WS}' CAMERA_TOPIC=/hbmem_img bash '${REMOTE_SCRIPT_DIR}/run_hik_cam_loop.sh'"

# wait for camera
camera_deadline=$((SECONDS + 12))
while [ "${SECONDS}" -lt "${camera_deadline}" ]; do
  if timeout 2 ros2 topic info /hbmem_img 2>/dev/null | grep -q "Publisher count: 1"; then
    break
  fi
  sleep 0.5
done

# --- start detection ---
tmux_cmd new-session -d -s rm_det \
  "env REMOTE_WS='${REMOTE_WS}' DETECTOR_TYPE=bear DETECTOR_TOPIC='${DETECTOR_TOPIC}' BEAR_LOG_LEVEL='${BEAR_LOG_LEVEL}' BEAR_SCORE_THRESHOLD='${BEAR_SCORE_THRESHOLD}' BEAR_STABLE_REQUIRED_HITS='${BEAR_STABLE_REQUIRED_HITS}' BEAR_STABLE_MATCH_RADIUS_PX='${BEAR_STABLE_MATCH_RADIUS_PX}' BEAR_STABLE_MAX_TRACK_AGE_MS='${BEAR_STABLE_MAX_TRACK_AGE_MS}' BEAR_PUBLISH_DEBUG_TEXT=false BEAR_LOG_DETECTIONS=false bash '${REMOTE_SCRIPT_DIR}/run_rm_det_loop.sh'"

# wait for detection topic
det_deadline=$((SECONDS + 10))
while [ "${SECONDS}" -lt "${det_deadline}" ]; do
  if timeout 2 ros2 topic info "${DETECTOR_TOPIC}" 2>/dev/null | grep -q "Publisher count: 1"; then
    break
  fi
  sleep 0.5
done

# --- start bridge ---
tmux_cmd new-session -d -s rm_bridge \
  "env REMOTE_WS='${REMOTE_WS}' SERIAL_PORT='${SERIAL_PORT}' DETECTOR_TOPIC='${DETECTOR_TOPIC}' ALLOWED_TARGET_TYPES='${ALLOWED_TARGET_TYPES}' BRIDGE_REQUIRE_VISION_ENABLED=true BRIDGE_LOG_DIAG_FEEDBACK=false BRIDGE_MIN_CONFIDENCE='${BRIDGE_MIN_CONFIDENCE}' ENABLE_FIXED_RATE_FOLLOW=true FOLLOW_PROFILE='${FOLLOW_PROFILE}' bash '${REMOTE_SCRIPT_DIR}/run_rm_bridge_loop.sh'"

echo ""
echo "=== headless follow light started (profile=${FOLLOW_PROFILE}) ==="
tmux_cmd ls || true

echo ""
echo "--- nodes ---"
sleep 3
ros2 node list 2>/dev/null || true

echo ""
echo "--- topic pub/sub ---"
ros2 topic info /hbmem_img 2>/dev/null || true
ros2 topic info /image_raw 2>/dev/null || true
ros2 topic info "${DETECTOR_TOPIC}" 2>/dev/null || true

echo ""
echo "--- top CPU ---"
ps -eo pid,pcpu,pmem,comm --sort=-pcpu | head -15
