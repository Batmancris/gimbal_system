#!/usr/bin/env bash
set -eo pipefail

REMOTE_WS="${REMOTE_WS:-/home/sunrise/rm_ws}"

cd "${REMOTE_WS}"
source /opt/tros/humble/setup.bash
source install/setup.bash || true

echo "[topics]"
ros2 topic list

echo
echo "[/hbmem_img]"
ros2 topic info /hbmem_img -v

echo
echo "[/dnn_node_sample]"
ros2 topic info /dnn_node_sample -v

echo
echo "[tmux rm_det]"
tmux capture-pane -pt rm_det || true

echo
echo "[tmux rm_bridge]"
tmux capture-pane -pt rm_bridge || true

echo
echo "[tmux hik_cam]"
tmux capture-pane -pt hik_cam || true
