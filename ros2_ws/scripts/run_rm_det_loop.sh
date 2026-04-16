#!/usr/bin/env bash
set +u
REMOTE_WS="${REMOTE_WS:-/home/sunrise/rm_ws}"
DETECTOR_TYPE="${DETECTOR_TYPE:-armor}"
DETECTOR_TOPIC="${DETECTOR_TOPIC:-/dnn_node_sample}"
VEHICLE_MODEL_PATH="${VEHICLE_MODEL_PATH:-}"
cd "${REMOTE_WS}" || exit 1
source /opt/tros/humble/setup.bash
source "${REMOTE_WS}/install/setup.bash" || true
while true; do
  printf '[%s] starting rm_det
' "$(date +%F_%T)"
  if [ "${DETECTOR_TYPE}" = "vehicle" ]; then
    ros2 run rm_vehicle_detection rm_vehicle_detection_node --ros-args \
      -p output_topic:="${DETECTOR_TOPIC}" \
      -p model_path:="${VEHICLE_MODEL_PATH}"
  else
    ros2 run rm_armor_detection rm_armor_detection
  fi
  rc=$?
  printf '[%s] rm_det exited rc=%s, restarting in 2s
' "$(date +%F_%T)" "${rc}"
  sleep 2
done
