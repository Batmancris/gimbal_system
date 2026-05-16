#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/tianaim_paths.sh"

cd "${TIANAIM_ROS_WS}"
source /opt/tros/humble/setup.bash
colcon build --packages-up-to hik_camera rm_bear_detection rm_gimbal_bridge
