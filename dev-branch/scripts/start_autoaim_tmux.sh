#!/usr/bin/env bash
set -eo pipefail

REMOTE_WS="${REMOTE_WS:-/home/sunrise/rm_ws}"
SERIAL_PORT="${SERIAL_PORT:-/dev/ttyS1}"
ENEMY_PREFIX="${ENEMY_PREFIX:-}"

cd "${REMOTE_WS}"
source /opt/tros/humble/setup.bash
source install/setup.bash || true

pkill -f "rm_armor_detection rm_armor_detection" 2>/dev/null || true
pkill -f rm_gimbal_bridge_node 2>/dev/null || true
pkill -f hik_camera_node 2>/dev/null || true

tmux kill-session -t rm_det 2>/dev/null || true
tmux kill-session -t rm_bridge 2>/dev/null || true
tmux kill-session -t hik_cam 2>/dev/null || true

bridge_enemy_arg=""
if [ -n "${ENEMY_PREFIX}" ]; then
  bridge_enemy_arg=" -p enemy_prefix:=${ENEMY_PREFIX}"
fi

tmux new-session -d -s rm_det \
  "bash -lc 'cd \"${REMOTE_WS}\" && source /opt/tros/humble/setup.bash && source install/setup.bash || true && exec ros2 run rm_armor_detection rm_armor_detection'"

tmux new-session -d -s rm_bridge \
  "bash -lc 'cd \"${REMOTE_WS}\" && source /opt/tros/humble/setup.bash && source install/setup.bash || true && exec ros2 run rm_gimbal_bridge rm_gimbal_bridge_node --ros-args -p serial_port:=${SERIAL_PORT}${bridge_enemy_arg}'"

tmux new-session -d -s hik_cam \
  "bash -lc 'cd \"${REMOTE_WS}\" && source /opt/tros/humble/setup.bash && source install/setup.bash || true && exec ros2 launch hik_camera hik_camera.launch.py'"

echo "tmux sessions started:"
tmux ls || true
