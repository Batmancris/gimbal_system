#!/usr/bin/env bash
set +u
REMOTE_WS="${REMOTE_WS:-/home/sunrise/rm_ws}"
CAMERA_TOPIC="${CAMERA_TOPIC:-/image_raw}"
CAMERA_WATCHDOG_ENABLE="${CAMERA_WATCHDOG_ENABLE:-false}"
CAMERA_FIRST_FRAME_TIMEOUT_SEC="${CAMERA_FIRST_FRAME_TIMEOUT_SEC:-8}"
CAMERA_WATCHDOG_INTERVAL_SEC="${CAMERA_WATCHDOG_INTERVAL_SEC:-2}"
CAMERA_FRAME_TIMEOUT_SEC="${CAMERA_FRAME_TIMEOUT_SEC:-5}"
cd "${REMOTE_WS}" || exit 1
source /opt/tros/humble/setup.bash
source "${REMOTE_WS}/install/setup.bash" || true

wait_for_camera_frame() {
  local hz_log
  hz_log="$(mktemp /tmp/hik_cam_watchdog.XXXXXX)"
  timeout "${CAMERA_FRAME_TIMEOUT_SEC}" ros2 topic hz "${CAMERA_TOPIC}" >"${hz_log}" 2>&1
  grep -q "average rate" "${hz_log}"
  local rc=$?
  rm -f "${hz_log}"
  return "${rc}"
}

stop_camera_launch() {
  local child_pid="$1"
  kill "${child_pid}" 2>/dev/null || true
  sleep 1
  pkill -P "${child_pid}" 2>/dev/null || true
  pkill -f '[h]ik_camera_node' 2>/dev/null || true
}

while true; do
  printf '[%s] starting hik_cam\n' "$(date +%F_%T)"
  ros2 launch hik_camera hik_camera.launch.py &
  cam_pid=$!

  frame_timeout_saved="${CAMERA_FRAME_TIMEOUT_SEC}"
  CAMERA_FRAME_TIMEOUT_SEC="${CAMERA_FIRST_FRAME_TIMEOUT_SEC}"
  wait_for_camera_frame
  first_frame_rc=$?
  CAMERA_FRAME_TIMEOUT_SEC="${frame_timeout_saved}"
  if [ "${first_frame_rc}" -ne 0 ]; then
    printf '[%s] hik_cam no first frame on %s, restarting\n' "$(date +%F_%T)" "${CAMERA_TOPIC}"
    stop_camera_launch "${cam_pid}"
    wait "${cam_pid}" 2>/dev/null
    sleep 2
    continue
  fi

  if [ "${CAMERA_WATCHDOG_ENABLE}" != "true" ]; then
    wait "${cam_pid}" 2>/dev/null
    rc=$?
    printf '[%s] hik_cam exited rc=%s, restarting in 2s\n' "$(date +%F_%T)" "${rc}"
    sleep 2
    continue
  fi

  while kill -0 "${cam_pid}" 2>/dev/null; do
    sleep "${CAMERA_WATCHDOG_INTERVAL_SEC}"
    if ! wait_for_camera_frame; then
      printf '[%s] hik_cam frame watchdog timeout on %s, restarting\n' "$(date +%F_%T)" "${CAMERA_TOPIC}"
      stop_camera_launch "${cam_pid}"
      break
    fi
  done
  wait "${cam_pid}" 2>/dev/null
  rc=$?
  printf '[%s] hik_cam exited rc=%s, restarting in 2s\n' "$(date +%F_%T)" "${rc}"
  sleep 2
done
