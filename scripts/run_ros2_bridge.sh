#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/tianaim_paths.sh"

cd "${TIANAIM_ROS_WS}"
source /opt/tros/humble/setup.bash
source install/setup.bash
ros2 launch rm_gimbal_bridge rm_gimbal_bridge.launch.py "$@"
