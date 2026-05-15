#!/usr/bin/env bash
set +u

REMOTE_WS="${REMOTE_WS:-/home/sunrise/rm_ws}"
SERIAL_PORT="${SERIAL_PORT:-/dev/serial/by-id/usb-Batmancris_Gimbal_Control_CDC_3162376B3439-if00}"
ENEMY_PREFIX="${ENEMY_PREFIX:-}"
DETECTOR_TOPIC="${DETECTOR_TOPIC:-/bear_detection/targets}"
ALLOWED_TARGET_TYPES="${ALLOWED_TARGET_TYPES:-bear}"
BRIDGE_DELAY_SEC="${BRIDGE_DELAY_SEC:-0}"
WAIT_FOR_SERIAL_SEC="${WAIT_FOR_SERIAL_SEC:-15}"
BRIDGE_REQUIRE_VISION_ENABLED="${BRIDGE_REQUIRE_VISION_ENABLED:-true}"
BRIDGE_LOG_DIAG_FEEDBACK="${BRIDGE_LOG_DIAG_FEEDBACK:-false}"
DETECTOR_READY_TIMEOUT_SEC="${DETECTOR_READY_TIMEOUT_SEC:-10}"
ENABLE_FIXED_RATE_FOLLOW="${ENABLE_FIXED_RATE_FOLLOW:-true}"
FOLLOW_PROFILE="${FOLLOW_PROFILE:-stable}"

# --- stable profile (hidden rollback default) ---
_stable() {
  FOLLOW_SEND_RATE_HZ=50.0
  FOLLOW_INTERP_RATE_HZ=10.0
  FOLLOW_CONTROL_MODE=light_predict
  FOLLOW_SMOOTHING_ALPHA=0.35
  FOLLOW_MAX_STEP_PX=36.0
  FOLLOW_DEADBAND_PX=5.0
  MEASUREMENT_JITTER_DEADBAND_PX=18.0
  FAST_FOLLOW_ERROR_PX=120.0
  FAST_FOLLOW_SMOOTHING_ALPHA=0.55
  FAST_FOLLOW_MAX_STEP_PX=72.0
  LIGHT_FOLLOW_GAIN=0.45
  PREDICT_ALPHA=0.65
  PREDICT_BETA=0.00
  PREDICT_HORIZON_SEC=0.00
  TARGET_HOLD_MS=350
  TARGET_SWITCH_RADIUS_PX=120.0
  TARGET_SWITCH_MIN_CONF_GAIN=0.30
  TARGET_SWITCH_CENTER_GAIN_PX=60.0
  MIN_SEND_DELTA_PX=2.0
  SEND_KEEPALIVE_MS=40
  BRIDGE_MIN_CONFIDENCE=0.71
  CENTER_GATE_X_RATIO=1.00
  CENTER_GATE_Y_RATIO=1.00
}

# --- fast_best: the one and only daily fast follow mode ---
_fast_best() {
  FOLLOW_SEND_RATE_HZ=80.0
  FOLLOW_INTERP_RATE_HZ=35.0
  FOLLOW_CONTROL_MODE=light_predict
  FOLLOW_SMOOTHING_ALPHA=0.50
  FOLLOW_MAX_STEP_PX=75.0
  FOLLOW_DEADBAND_PX=5.0
  MEASUREMENT_JITTER_DEADBAND_PX=10.0
  FAST_FOLLOW_ERROR_PX=95.0
  FAST_FOLLOW_SMOOTHING_ALPHA=0.62
  FAST_FOLLOW_MAX_STEP_PX=105.0
  LIGHT_FOLLOW_GAIN=0.58
  PREDICT_ALPHA=0.65
  PREDICT_BETA=0.03
  PREDICT_HORIZON_SEC=0.025
  TARGET_HOLD_MS=240
  TARGET_SWITCH_RADIUS_PX=200.0
  TARGET_SWITCH_MIN_CONF_GAIN=0.30
  TARGET_SWITCH_CENTER_GAIN_PX=60.0
  MIN_SEND_DELTA_PX=1.0
  SEND_KEEPALIVE_MS=20
  BRIDGE_MIN_CONFIDENCE=0.71
  CENTER_GATE_X_RATIO=1.00
  CENTER_GATE_Y_RATIO=1.00
}

# Select profile
_resolved="${FOLLOW_PROFILE}"
case "${FOLLOW_PROFILE}" in
  stable)       _stable ;;
  fast_best)    _fast_best ;;
  fast|fast_safe|fast_current)
    printf '[%s] NOTE: %s is now an alias for fast_best\n' "$(date +%F_%T)" "${FOLLOW_PROFILE}"
    _resolved="fast_best"
    _fast_best ;;
  fast_aggressive)
    printf '[%s] WARNING: fast_aggressive is deprecated, using fast_best\n' "$(date +%F_%T)"
    _resolved="fast_best"
    _fast_best ;;
  *)
    printf '[%s] WARNING: unknown FOLLOW_PROFILE="%s", falling back to stable\n' "$(date +%F_%T)" "${FOLLOW_PROFILE}"
    _resolved="stable"
    _stable ;;
esac

# --- Startup banner ---
printf '============================================\n'
printf '[%s] rm_bridge starting\n' "$(date +%F_%T)"
printf '  profile:                      %s\n' "${_resolved}"
printf '  require_lower_vision_enabled: %s\n' "${BRIDGE_REQUIRE_VISION_ENABLED}"
printf '  follow_send_rate_hz:          %s\n' "${FOLLOW_SEND_RATE_HZ}"
printf '  follow_interp_rate_hz:        %s\n' "${FOLLOW_INTERP_RATE_HZ}"
printf '  follow_max_step_px:           %s\n' "${FOLLOW_MAX_STEP_PX}"
printf '  fast_follow_max_step_px:      %s\n' "${FAST_FOLLOW_MAX_STEP_PX}"
printf '  light_follow_gain:            %s\n' "${LIGHT_FOLLOW_GAIN}"
printf '  predict_beta:                 %s\n' "${PREDICT_BETA}"
printf '  predict_horizon_sec:          %s\n' "${PREDICT_HORIZON_SEC}"
printf '  min_confidence:               %s\n' "${BRIDGE_MIN_CONFIDENCE}"
printf '============================================\n'

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
  if timeout 5 ros2 topic info "${DETECTOR_TOPIC}" 2>/dev/null | grep -q "Publisher count: 1"; then
    break
  fi
  sleep 0.5
done

while true; do
  printf '[%s] starting rm_bridge (profile=%s)\n' "$(date +%F_%T)" "${_resolved}"
  bridge_args=(
    --ros-args
    -p "input_topic:=${DETECTOR_TOPIC}"
    -p "serial_port:=${SERIAL_PORT}"
    -p "allowed_target_types:=['${ALLOWED_TARGET_TYPES}']"
    -p "require_lower_vision_enabled:=${BRIDGE_REQUIRE_VISION_ENABLED}"
    -p "log_diag_feedback:=${BRIDGE_LOG_DIAG_FEEDBACK}"
    -p "enable_fixed_rate_follow:=${ENABLE_FIXED_RATE_FOLLOW}"
    -p "follow_send_rate_hz:=${FOLLOW_SEND_RATE_HZ}"
    -p "follow_interp_rate_hz:=${FOLLOW_INTERP_RATE_HZ}"
    -p "follow_control_mode:=${FOLLOW_CONTROL_MODE}"
    -p "follow_smoothing_alpha:=${FOLLOW_SMOOTHING_ALPHA}"
    -p "follow_max_step_px:=${FOLLOW_MAX_STEP_PX}"
    -p "follow_deadband_px:=${FOLLOW_DEADBAND_PX}"
    -p "measurement_jitter_deadband_px:=${MEASUREMENT_JITTER_DEADBAND_PX}"
    -p "fast_follow_error_px:=${FAST_FOLLOW_ERROR_PX}"
    -p "fast_follow_smoothing_alpha:=${FAST_FOLLOW_SMOOTHING_ALPHA}"
    -p "fast_follow_max_step_px:=${FAST_FOLLOW_MAX_STEP_PX}"
    -p "light_follow_gain:=${LIGHT_FOLLOW_GAIN}"
    -p "predict_alpha:=${PREDICT_ALPHA}"
    -p "predict_beta:=${PREDICT_BETA}"
    -p "predict_horizon_sec:=${PREDICT_HORIZON_SEC}"
    -p "target_hold_ms:=${TARGET_HOLD_MS}"
    -p "target_switch_radius_px:=${TARGET_SWITCH_RADIUS_PX}"
    -p "target_switch_min_conf_gain:=${TARGET_SWITCH_MIN_CONF_GAIN}"
    -p "target_switch_center_gain_px:=${TARGET_SWITCH_CENTER_GAIN_PX}"
    -p "min_send_delta_px:=${MIN_SEND_DELTA_PX}"
    -p "send_keepalive_ms:=${SEND_KEEPALIVE_MS}"
    -p "min_confidence:=${BRIDGE_MIN_CONFIDENCE}"
    -p "center_gate_x_ratio:=${CENTER_GATE_X_RATIO}"
    -p "center_gate_y_ratio:=${CENTER_GATE_Y_RATIO}"
  )
  if [ -n "${ENEMY_PREFIX}" ]; then
    bridge_args+=(-p "enemy_prefix:=${ENEMY_PREFIX}")
  fi
  ros2 run rm_gimbal_bridge rm_gimbal_bridge_node "${bridge_args[@]}"
  rc=$?
  printf '[%s] rm_bridge exited rc=%s, restarting in 2s\n' "$(date +%F_%T)" "${rc}"
  sleep 2
done
