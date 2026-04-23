#!/usr/bin/env bash
set -euo pipefail

systemctl --user stop rm-autoaim.service rm-bridge.service

tmux -L autoaim kill-session -t rm_det 2>/dev/null || true
tmux -L autoaim kill-session -t hik_cam 2>/dev/null || true
tmux -L autoaim kill-session -t rm_vis 2>/dev/null || true
tmux -L bridge kill-session -t rm_bridge 2>/dev/null || true

pkill -f "[r]m_armor_detection rm_armor_detection" 2>/dev/null || true
pkill -f "[r]m_vehicle_detection rm_vehicle_detection_node" 2>/dev/null || true
pkill -f "[r]m_bear_detection rm_bear_detection_node" 2>/dev/null || true
pkill -f "[h]ik_camera_node" 2>/dev/null || true
pkill -f "[r]m_gimbal_bridge_node" 2>/dev/null || true

echo "autoaim stack stopped"
