#!/usr/bin/env bash
set +u

REMOTE_WS="${REMOTE_WS:-/home/sunrise/rm_ws}"
DETECTOR_TYPE="${DETECTOR_TYPE:-bear}"
DETECTOR_TOPIC="${DETECTOR_TOPIC:-/bear_detection/targets}"
VEHICLE_MODEL_PATH="${VEHICLE_MODEL_PATH:-}"
VEHICLE_BOX_FORMAT="${VEHICLE_BOX_FORMAT:-cxcywh}"
VEHICLE_SCORE_THRESHOLD="${VEHICLE_SCORE_THRESHOLD:-0.10}"
VEHICLE_PUBLISH_DEBUG_TEXT="${VEHICLE_PUBLISH_DEBUG_TEXT:-false}"
VEHICLE_LOG_LEVEL="${VEHICLE_LOG_LEVEL:-warn}"
BEAR_LOG_LEVEL="${BEAR_LOG_LEVEL:-info}"
BEAR_SCORE_THRESHOLD="${BEAR_SCORE_THRESHOLD:-0.71}"
BEAR_STABLE_REQUIRED_HITS="${BEAR_STABLE_REQUIRED_HITS:-2}"
BEAR_STABLE_MATCH_RADIUS_PX="${BEAR_STABLE_MATCH_RADIUS_PX:-140.0}"
BEAR_STABLE_MAX_TRACK_AGE_MS="${BEAR_STABLE_MAX_TRACK_AGE_MS:-200}"
BEAR_PUBLISH_DEBUG_TEXT="${BEAR_PUBLISH_DEBUG_TEXT:-false}"
BEAR_LOG_DETECTIONS="${BEAR_LOG_DETECTIONS:-false}"

cd "${REMOTE_WS}" || exit 1
source /opt/tros/humble/setup.bash
source "${REMOTE_WS}/install/setup.bash" || true

while true; do
  printf '[%s] starting rm_det\n' "$(date +%F_%T)"
  if [ "${DETECTOR_TYPE}" = "bear" ]; then
    ros2 run rm_bear_detection rm_bear_detection_node --ros-args \
      --log-level "${BEAR_LOG_LEVEL}" \
      -p output_topic:="${DETECTOR_TOPIC}" \
      -p score_threshold:="${BEAR_SCORE_THRESHOLD}" \
      -p stable_required_hits:="${BEAR_STABLE_REQUIRED_HITS}" \
      -p stable_match_radius_px:="${BEAR_STABLE_MATCH_RADIUS_PX}" \
      -p stable_max_track_age_ms:="${BEAR_STABLE_MAX_TRACK_AGE_MS}" \
      -p publish_debug_text:="${BEAR_PUBLISH_DEBUG_TEXT}" \
      -p log_detections:="${BEAR_LOG_DETECTIONS}"
  elif [ "${DETECTOR_TYPE}" = "vehicle" ]; then
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
