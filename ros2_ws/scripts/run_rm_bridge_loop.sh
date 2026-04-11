#!/usr/bin/env bash
set +u

REMOTE_WS="${REMOTE_WS:-/home/sunrise/rm_ws}"
SERIAL_PORT="${SERIAL_PORT:-/dev/serial/by-id/usb-Batmancris_Gimbal_Control_CDC_3162376B3439-if00}"
ENEMY_PREFIX="${ENEMY_PREFIX:-blue_}"
BRIDGE_DELAY_SEC="${BRIDGE_DELAY_SEC:-0}"
WAIT_FOR_SERIAL_SEC="${WAIT_FOR_SERIAL_SEC:-15}"
BRIDGE_REQUIRE_VISION_ENABLED="${BRIDGE_REQUIRE_VISION_ENABLED:-false}"
DETECTOR_READY_TIMEOUT_SEC="${DETECTOR_READY_TIMEOUT_SEC:-10}"

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
  if ros2 topic info /dnn_node_sample 2>/dev/null | grep -q "Publisher count: 1"; then
    break
  fi
  sleep 0.5
done

while true; do
  printf '[%s] starting rm_bridge
' "$(date +%F_%T)"
  ros2 run rm_gimbal_bridge rm_gimbal_bridge_node --ros-args     -p serial_port:="${SERIAL_PORT}"     -p enemy_prefix:="${ENEMY_PREFIX}"     -p require_lower_vision_enabled:="${BRIDGE_REQUIRE_VISION_ENABLED}"
  rc=$?
  printf '[%s] rm_bridge exited rc=%s, restarting in 2s
' "$(date +%F_%T)" "${rc}"
  sleep 2
done
