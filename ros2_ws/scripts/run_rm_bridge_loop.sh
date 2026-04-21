#!/usr/bin/env bash
set +u

REMOTE_WS="${REMOTE_WS:-/home/sunrise/rm_ws}"
SERIAL_PORT="${SERIAL_PORT:-/dev/serial/by-id/usb-Batmancris_Gimbal_Control_CDC_3162376B3439-if00}"
ENEMY_PREFIX="${ENEMY_PREFIX:-}"
DETECTOR_TOPIC="${DETECTOR_TOPIC:-/vehicle_detection/targets}"
BRIDGE_DELAY_SEC="${BRIDGE_DELAY_SEC:-0}"
WAIT_FOR_SERIAL_SEC="${WAIT_FOR_SERIAL_SEC:-15}"
BRIDGE_REQUIRE_VISION_ENABLED="${BRIDGE_REQUIRE_VISION_ENABLED:-true}"
DETECTOR_READY_TIMEOUT_SEC="${DETECTOR_READY_TIMEOUT_SEC:-10}"
FOLLOW_CONTROL_MODE="${FOLLOW_CONTROL_MODE:-light_predict}"
FOLLOW_SMOOTHING_ALPHA="${FOLLOW_SMOOTHING_ALPHA:-0.65}"
FOLLOW_MAX_STEP_PX="${FOLLOW_MAX_STEP_PX:-72.0}"
FOLLOW_DEADBAND_PX="${FOLLOW_DEADBAND_PX:-6.0}"
LIGHT_FOLLOW_GAIN="${LIGHT_FOLLOW_GAIN:-0.95}"
PREDICT_ALPHA="${PREDICT_ALPHA:-0.55}"
PREDICT_BETA="${PREDICT_BETA:-0.10}"
PREDICT_HORIZON_SEC="${PREDICT_HORIZON_SEC:-0.03}"

cd "${REMOTE_WS}" || exit 1
source /opt/tros/humble/setup.bash
source "${REMOTE_WS}/install/setup.bash" || true

if [ "${BRIDGE_DELAY_SEC}" -gt 0 ] 2>/dev/null; then
  sleep "${BRIDGE_DELAY_SEC}"
fi

serial_deadline=$((SECONDS + WAIT_FOR_SERIAL_SEC))
while [ ! -e "${SERIAL_PORT}" ] && [ "${SECONDS}" -lt "${serial_deadline}" ]; do
  sleep 0.5
done

det_deadline=$((SECONDS + DETECTOR_READY_TIMEOUT_SEC))
while [ "${SECONDS}" -lt "${det_deadline}" ]; do
  if ros2 topic info "${DETECTOR_TOPIC}" 2>/dev/null | grep -q "Publisher count: 1"; then
    break
  fi
  sleep 0.5
done

while true; do
  printf '[%s] starting rm_bridge
' "$(date +%F_%T)"
  bridge_args=(
    --ros-args
    -p "input_topic:=${DETECTOR_TOPIC}"
    -p "serial_port:=${SERIAL_PORT}"
    -p "allowed_target_types:=['vehicle']"
    -p "require_lower_vision_enabled:=${BRIDGE_REQUIRE_VISION_ENABLED}"
    -p "follow_control_mode:=${FOLLOW_CONTROL_MODE}"
    -p "follow_smoothing_alpha:=${FOLLOW_SMOOTHING_ALPHA}"
    -p "follow_max_step_px:=${FOLLOW_MAX_STEP_PX}"
    -p "follow_deadband_px:=${FOLLOW_DEADBAND_PX}"
    -p "light_follow_gain:=${LIGHT_FOLLOW_GAIN}"
    -p "predict_alpha:=${PREDICT_ALPHA}"
    -p "predict_beta:=${PREDICT_BETA}"
    -p "predict_horizon_sec:=${PREDICT_HORIZON_SEC}"
  )
  if [ -n "${ENEMY_PREFIX}" ]; then
    bridge_args+=(-p "enemy_prefix:=${ENEMY_PREFIX}")
  fi
  ros2 run rm_gimbal_bridge rm_gimbal_bridge_node "${bridge_args[@]}"
  rc=$?
  printf '[%s] rm_bridge exited rc=%s, restarting in 2s
' "$(date +%F_%T)" "${rc}"
  sleep 2
done
