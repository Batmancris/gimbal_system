#!/usr/bin/env bash
set +u
REMOTE_WS="${REMOTE_WS:-/home/sunrise/rm_ws}"
cd "${REMOTE_WS}" || exit 1
source /opt/tros/humble/setup.bash
source "${REMOTE_WS}/install/setup.bash" || true
while true; do
  printf '[%s] starting rm_det
' "$(date +%F_%T)"
  ros2 run rm_armor_detection rm_armor_detection
  rc=$?
  printf '[%s] rm_det exited rc=%s, restarting in 2s
' "$(date +%F_%T)" "${rc}"
  sleep 2
done
