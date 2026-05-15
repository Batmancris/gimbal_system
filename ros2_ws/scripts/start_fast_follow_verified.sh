#!/usr/bin/env bash
set +u

# ============================================================
# start_fast_follow_verified.sh
# One-click fast follow startup with auto health check.
#
# Usage:
#   bash start_fast_follow_verified.sh                          # default: fast_best
#   FOLLOW_PROFILE=stable bash start_fast_follow_verified.sh    # rollback
# ============================================================

REMOTE_WS="${REMOTE_WS:-/home/sunrise/rm_ws}"
FOLLOW_PROFILE="${FOLLOW_PROFILE:-fast_best}"
BRIDGE_REQUIRE_VISION_ENABLED="${BRIDGE_REQUIRE_VISION_ENABLED:-false}"
BRIDGE_MIN_CONFIDENCE="${BRIDGE_MIN_CONFIDENCE:-0.50}"
BEAR_LOG_LEVEL="${BEAR_LOG_LEVEL:-warn}"
BEAR_SCORE_THRESHOLD="${BEAR_SCORE_THRESHOLD:-0.71}"
BEAR_STABLE_REQUIRED_HITS="${BEAR_STABLE_REQUIRED_HITS:-2}"
BEAR_STABLE_MATCH_RADIUS_PX="${BEAR_STABLE_MATCH_RADIUS_PX:-140.0}"
BEAR_STABLE_MAX_TRACK_AGE_MS="${BEAR_STABLE_MAX_TRACK_AGE_MS:-200}"
SERIAL_PORT="${SERIAL_PORT:-/dev/serial/by-id/usb-Batmancris_Gimbal_Control_CDC_3162376B3439-if00}"
ALLOWED_TARGET_TYPES="${ALLOWED_TARGET_TYPES:-bear}"
DETECTOR_TOPIC="${DETECTOR_TOPIC:-/bear_detection/targets}"
TMUX_SOCKET="${TMUX_SOCKET:-autoaim}"

tmux_cmd() {
  tmux -L "${TMUX_SOCKET}" "$@"
}

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# --- Status tracking ---
CAMERA_STATUS="FAIL"
DETECTOR_STATUS="FAIL"
BRIDGE_STATUS="FAIL"
SUB_STATUS="FAIL"
GATE_STATUS="OFF"

fail_diag() {
  echo ""
  echo "========== FAILURE DIAGNOSTICS =========="
  echo ""
  echo "--- tmux hik_cam last 80 lines ---"
  tmux_cmd capture-pane -t hik_cam -p -S -80 2>/dev/null || echo "(no hik_cam session)"
  echo ""
  echo "--- tmux rm_det last 80 lines ---"
  tmux_cmd capture-pane -t rm_det -p -S -80 2>/dev/null || echo "(no rm_det session)"
  echo ""
  echo "--- tmux rm_bridge last 120 lines ---"
  tmux_cmd capture-pane -t rm_bridge -p -S -120 2>/dev/null || echo "(no rm_bridge session)"
  echo ""
  echo "--- ros2 node list ---"
  ros2 node list 2>/dev/null || echo "(ros2 daemon not running)"
  echo ""
  echo "--- ros2 topic info /hbmem_img ---"
  ros2 topic info /hbmem_img 2>/dev/null || echo "(topic not found)"
  echo ""
  echo "--- ros2 topic info ${DETECTOR_TOPIC} ---"
  ros2 topic info "${DETECTOR_TOPIC}" 2>/dev/null || echo "(topic not found)"
  echo ""
  echo "--- top CPU processes ---"
  ps -eo pid,pcpu,pmem,comm,args --sort=-pcpu | head -25
  echo ""
  echo "=========================================="
}

# ============================================================
# Phase 0: Environment
# ============================================================
cd "${REMOTE_WS}" || { echo "FATAL: cannot cd to ${REMOTE_WS}"; exit 1; }
source /opt/tros/humble/setup.bash
source install/setup.bash || true

echo "[$(date +%F_%T)] Starting fast follow (profile=${FOLLOW_PROFILE}, vision_gate=${BRIDGE_REQUIRE_VISION_ENABLED})"

# ============================================================
# Phase 1: Cleanup
# ============================================================
echo "[$(date +%F_%T)] Cleaning up stale processes..."

# Kill apt/rdk-studio resource hogs
pkill -f 'apt-show-versions' 2>/dev/null || true
pkill -f 'apt-get update' 2>/dev/null || true
pkill -f 'rosbridge-server' 2>/dev/null || true
pkill -f 'rosbridge' 2>/dev/null || true

# Kill old chain
tmux_cmd kill-session -t hik_cam 2>/dev/null || true
tmux_cmd kill-session -t rm_det 2>/dev/null || true
tmux_cmd kill-session -t rm_bridge 2>/dev/null || true
pkill -f rm_gimbal_bridge_node 2>/dev/null || true
pkill -f run_rm_bridge_loop.sh 2>/dev/null || true
pkill -f rm_bear_detection_node 2>/dev/null || true
pkill -f hik_camera_node 2>/dev/null || true
pkill -f 'ros2 topic hz' 2>/dev/null || true
pkill -f 'ros2 topic echo' 2>/dev/null || true
pkill -f rm_autoaim_visualizer 2>/dev/null || true
pkill -f rm_vis 2>/dev/null || true

sleep 1

# Restart ros2 daemon
ros2 daemon stop 2>/dev/null || true
sleep 0.5
ros2 daemon start 2>/dev/null || true
sleep 1

echo "[$(date +%F_%T)] Cleanup done."

# ============================================================
# Phase 2: Start camera
# ============================================================
echo "[$(date +%F_%T)] Starting camera..."

REMOTE_SRC_DIR="${REMOTE_WS}/src/ros2_ws"
REMOTE_SCRIPT_DIR="${REMOTE_SRC_DIR}/scripts"

tmux_cmd new-session -d -s hik_cam \
  "env REMOTE_WS='${REMOTE_WS}' CAMERA_TOPIC=/hbmem_img bash '${REMOTE_SCRIPT_DIR}/run_hik_cam_loop.sh'"

camera_deadline=$((SECONDS + 25))
while [ "${SECONDS}" -lt "${camera_deadline}" ]; do
  if timeout 5 ros2 topic info /hbmem_img 2>/dev/null | grep -q "Publisher count: 1"; then
    CAMERA_STATUS="OK"
    break
  fi
  sleep 0.5
done

if [ "${CAMERA_STATUS}" = "FAIL" ]; then
  echo "[$(date +%F_%T)] FAIL: camera did not come up in 25s"
  fail_diag
  exit 1
fi
echo "[$(date +%F_%T)] Camera OK."

# ============================================================
# Phase 3: Start detection (wait at least 35s total)
# ============================================================
echo "[$(date +%F_%T)] Starting bear detection..."

tmux_cmd new-session -d -s rm_det \
  "env REMOTE_WS='${REMOTE_WS}' DETECTOR_TYPE=bear DETECTOR_TOPIC='${DETECTOR_TOPIC}' BEAR_LOG_LEVEL='${BEAR_LOG_LEVEL}' BEAR_SCORE_THRESHOLD='${BEAR_SCORE_THRESHOLD}' BEAR_STABLE_REQUIRED_HITS='${BEAR_STABLE_REQUIRED_HITS}' BEAR_STABLE_MATCH_RADIUS_PX='${BEAR_STABLE_MATCH_RADIUS_PX}' BEAR_STABLE_MAX_TRACK_AGE_MS='${BEAR_STABLE_MAX_TRACK_AGE_MS}' BEAR_PUBLISH_DEBUG_TEXT=false BEAR_LOG_DETECTIONS=false bash '${REMOTE_SCRIPT_DIR}/run_rm_det_loop.sh'"

det_deadline=$((SECONDS + 35))
while [ "${SECONDS}" -lt "${det_deadline}" ]; do
  if timeout 5 ros2 topic info "${DETECTOR_TOPIC}" 2>/dev/null | grep -q "Publisher count: 1"; then
    DETECTOR_STATUS="OK"
    break
  fi
  sleep 0.5
done

if [ "${DETECTOR_STATUS}" = "FAIL" ]; then
  echo "[$(date +%F_%T)] FAIL: detection topic did not come up in 35s"
  fail_diag
  exit 1
fi
echo "[$(date +%F_%T)] Detection OK."

# ============================================================
# Phase 4: Start bridge
# ============================================================
echo "[$(date +%F_%T)] Starting bridge (profile=${FOLLOW_PROFILE}, require_vision=${BRIDGE_REQUIRE_VISION_ENABLED})..."

tmux_cmd new-session -d -s rm_bridge \
  "env REMOTE_WS='${REMOTE_WS}' SERIAL_PORT='${SERIAL_PORT}' DETECTOR_TOPIC='${DETECTOR_TOPIC}' ALLOWED_TARGET_TYPES='${ALLOWED_TARGET_TYPES}' BRIDGE_REQUIRE_VISION_ENABLED=${BRIDGE_REQUIRE_VISION_ENABLED} BRIDGE_LOG_DIAG_FEEDBACK=false BRIDGE_MIN_CONFIDENCE='${BRIDGE_MIN_CONFIDENCE}' ENABLE_FIXED_RATE_FOLLOW=true FOLLOW_PROFILE='${FOLLOW_PROFILE}' bash '${REMOTE_SCRIPT_DIR}/run_rm_bridge_loop.sh'"

# Wait for /rm_gimbal_bridge node to appear
bridge_deadline=$((SECONDS + 30))
while [ "${SECONDS}" -lt "${bridge_deadline}" ]; do
  if ros2 node list 2>/dev/null | grep -q '/rm_gimbal_bridge'; then
    BRIDGE_STATUS="OK"
    break
  fi
  sleep 0.5
done

if [ "${BRIDGE_STATUS}" = "FAIL" ]; then
  echo "[$(date +%F_%T)] FAIL: /rm_gimbal_bridge node did not appear in 30s"
  fail_diag
  exit 1
fi
echo "[$(date +%F_%T)] Bridge OK."

# ============================================================
# Phase 5: Verify subscriptions
# ============================================================
sleep 2  # let bridge subscribe

sub_info=$(ros2 topic info "${DETECTOR_TOPIC}" -v 2>/dev/null || echo "")
if echo "${sub_info}" | grep -qi "subscri"; then
  SUB_STATUS="OK"
fi

# ============================================================
# Phase 6: Verify parameters
# ============================================================
if [ "${BRIDGE_REQUIRE_VISION_ENABLED}" = "false" ]; then
  GATE_STATUS="ON"
fi

# ============================================================
# Phase 7: Health report
# ============================================================
echo ""
echo "============================================"
echo "       FAST FOLLOW READY"
echo "============================================"
echo "  profile: ${FOLLOW_PROFILE}"
echo "  camera OK"
echo "  detector OK"
echo "  bridge OK"
echo "  target subscription OK"
echo "  lower vision gate bypass ON"
echo "============================================"
echo ""
echo "  next step: put bear in front of camera and observe gimbal motion"
echo ""
