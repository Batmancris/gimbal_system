#!/usr/bin/env bash
set +u

REMOTE_WS="${REMOTE_WS:-/home/sunrise/rm_ws}"
DETECTOR_TYPE="${DETECTOR_TYPE:-armor}"
DETECTOR_TOPIC="${DETECTOR_TOPIC:-/dnn_node_sample}"
VEHICLE_MODEL_PATH="${VEHICLE_MODEL_PATH:-}"
VEHICLE_BOX_FORMAT="${VEHICLE_BOX_FORMAT:-cxcywh}"
VEHICLE_SCORE_THRESHOLD="${VEHICLE_SCORE_THRESHOLD:-0.10}"
VEHICLE_PUBLISH_DEBUG_TEXT="${VEHICLE_PUBLISH_DEBUG_TEXT:-false}"
VEHICLE_LOG_LEVEL="${VEHICLE_LOG_LEVEL:-warn}"

cd "${REMOTE_WS}" || exit 1
source /opt/tros/humble/setup.bash
source "${REMOTE_WS}/install/setup.bash" || true

while true; do
  printf '[%s] starting rm_det\n' "$(date +%F_%T)"
  if [ "${DETECTOR_TYPE}" = "vehicle" ]; then
    if [ -n "${VEHICLE_MODEL_PATH}" ]; then
      ros2 run rm_vehicle_detection rm_vehicle_detection_node --ros-args \
        --log-level "${VEHICLE_LOG_LEVEL}" \
        -p output_topic:="${DETECTOR_TOPIC}" \
        -p model_path:="${VEHICLE_MODEL_PATH}" \
        -p box_format:="${VEHICLE_BOX_FORMAT}" \
        -p score_threshold:="${VEHICLE_SCORE_THRESHOLD}" \
        -p publish_debug_text:="${VEHICLE_PUBLISH_DEBUG_TEXT}"
    else
      ros2 run rm_vehicle_detection rm_vehicle_detection_node --ros-args \
        --log-level "${VEHICLE_LOG_LEVEL}" \
        -p output_topic:="${DETECTOR_TOPIC}" \
        -p box_format:="${VEHICLE_BOX_FORMAT}" \
        -p score_threshold:="${VEHICLE_SCORE_THRESHOLD}" \
        -p publish_debug_text:="${VEHICLE_PUBLISH_DEBUG_TEXT}"
    fi
  else
    ros2 run rm_armor_detection rm_armor_detection
  fi
  rc=$?
  printf '[%s] rm_det exited rc=%s, restarting in 2s\n' "$(date +%F_%T)" "${rc}"
  sleep 2
done
