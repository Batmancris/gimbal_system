#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

cd "${REPO_ROOT}/dev-branch"
source /opt/tros/humble/setup.bash
colcon build --packages-select hik_camera rm_armor_detection rm_gimbal_bridge
