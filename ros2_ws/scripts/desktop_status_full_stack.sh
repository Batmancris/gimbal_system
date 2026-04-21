#!/usr/bin/env bash
set -euo pipefail

echo "services:"
systemctl --user --no-pager --full status rm-autoaim.service rm-bridge.service | sed -n '1,80p'
echo
echo "tmux:"
tmux -L autoaim ls 2>/dev/null || true
tmux -L bridge ls 2>/dev/null || true
echo
echo "processes:"
ps -ef | grep -E 'hik_camera|rm_vehicle_detection|rm_gimbal_bridge|rm_armor_detection_visualizer|tmux' | grep -v grep || true
