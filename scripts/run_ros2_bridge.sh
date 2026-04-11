#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

cd "${REPO_ROOT}/dev-branch"
source /opt/tros/humble/setup.bash
source install/setup.bash
ros2 launch rm_gimbal_bridge rm_gimbal_bridge.launch.py "$@"
