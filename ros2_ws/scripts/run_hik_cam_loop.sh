#!/usr/bin/env bash
set +u
REMOTE_WS="${REMOTE_WS:-/home/sunrise/rm_ws}"
cd "${REMOTE_WS}" || exit 1
source /opt/tros/humble/setup.bash
source "${REMOTE_WS}/install/setup.bash" || true
while true; do
  printf '[%s] starting hik_cam
' "$(date +%F_%T)"
  ros2 launch hik_camera hik_camera.launch.py
  rc=$?
  printf '[%s] hik_cam exited rc=%s, restarting in 2s
' "$(date +%F_%T)" "${rc}"
  sleep 2
done
