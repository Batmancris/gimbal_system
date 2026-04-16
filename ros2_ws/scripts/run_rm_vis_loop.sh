#!/usr/bin/env bash
set +u
REMOTE_WS="${REMOTE_WS:-/home/sunrise/rm_ws}"
DISPLAY_VALUE="${DISPLAY_VALUE:-:0}"
XAUTHORITY_VALUE="${XAUTHORITY_VALUE:-/home/sunrise/.Xauthority}"
XDG_RUNTIME_DIR_VALUE="${XDG_RUNTIME_DIR_VALUE:-/run/user/1000}"
VIS_DELAY_SEC="${VIS_DELAY_SEC:-3}"
DETECTOR_TOPIC="${DETECTOR_TOPIC:-/dnn_node_sample}"
cd "${REMOTE_WS}" || exit 1
source /opt/tros/humble/setup.bash
source "${REMOTE_WS}/install/setup.bash" || true
if [ "${VIS_DELAY_SEC}" -gt 0 ] 2>/dev/null; then
  sleep "${VIS_DELAY_SEC}"
fi
while true; do
  export DISPLAY="${DISPLAY_VALUE}"
  export XAUTHORITY="${XAUTHORITY_VALUE}"
  export XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR_VALUE}"
  printf '[%s] starting rm_vis
' "$(date +%F_%T)"
  ros2 run rm_armor_detection rm_armor_detection_visualizer --ros-args \
    -p image_topic:=/image_raw \
    -p targets_topic:="${DETECTOR_TOPIC}"
  rc=$?
  printf '[%s] rm_vis exited rc=%s, restarting in 2s
' "$(date +%F_%T)" "${rc}"
  sleep 2
done
