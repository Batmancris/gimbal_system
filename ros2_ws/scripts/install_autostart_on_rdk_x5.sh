#!/usr/bin/env bash
set -euo pipefail

RDK_USER="${RDK_USER:-sunrise}"
RDK_HOST="${RDK_HOST:?Please set RDK_HOST, e.g. export RDK_HOST=192.168.127.10}"
RDK_PORT="${RDK_PORT:-22}"
REMOTE_WS="${REMOTE_WS:-/home/sunrise/rm_ws}"
REMOTE_SRC_DIR="${REMOTE_SRC_DIR:-${REMOTE_WS}/src/ros2_ws}"
REMOTE_SCRIPT_DIR="${REMOTE_SCRIPT_DIR:-${REMOTE_SRC_DIR}/scripts}"
LOCAL_WS="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

echo "Syncing autostart files to ${RDK_USER}@${RDK_HOST}:${REMOTE_WS}"

ssh -p "${RDK_PORT}" "${RDK_USER}@${RDK_HOST}" "mkdir -p '${REMOTE_SCRIPT_DIR}' ~/.config/systemd/user"

rsync -avz \
  "${LOCAL_WS}/scripts/clean_build_and_start_on_rdk.sh" \
  "${LOCAL_WS}/scripts/start_autoaim_tmux.sh" \
  "${LOCAL_WS}/scripts/start_rm_bridge_tmux.sh" \
  "${LOCAL_WS}/scripts/check_autoaim_topics.sh" \
  "${LOCAL_WS}/scripts/run_hik_cam_loop.sh" \
  "${LOCAL_WS}/scripts/run_rm_bridge_loop.sh" \
  "${LOCAL_WS}/scripts/run_rm_det_loop.sh" \
  "${LOCAL_WS}/scripts/run_rm_vis_loop.sh" \
  "${LOCAL_WS}/scripts/rm-autoaim.service" \
  "${LOCAL_WS}/scripts/rm-bridge.service" \
  "${RDK_USER}@${RDK_HOST}:${REMOTE_SCRIPT_DIR}/"

ssh -p "${RDK_PORT}" "${RDK_USER}@${RDK_HOST}" "\
  chmod +x '${REMOTE_SCRIPT_DIR}/clean_build_and_start_on_rdk.sh' \
           '${REMOTE_SCRIPT_DIR}/start_autoaim_tmux.sh' \
           '${REMOTE_SCRIPT_DIR}/start_rm_bridge_tmux.sh' \
           '${REMOTE_SCRIPT_DIR}/check_autoaim_topics.sh' \
           '${REMOTE_SCRIPT_DIR}/run_hik_cam_loop.sh' \
           '${REMOTE_SCRIPT_DIR}/run_rm_bridge_loop.sh' \
           '${REMOTE_SCRIPT_DIR}/run_rm_det_loop.sh' \
           '${REMOTE_SCRIPT_DIR}/run_rm_vis_loop.sh' && \
  cp '${REMOTE_SCRIPT_DIR}/rm-autoaim.service' ~/.config/systemd/user/rm-autoaim.service && \
  cp '${REMOTE_SCRIPT_DIR}/rm-bridge.service' ~/.config/systemd/user/rm-bridge.service && \
  systemctl --user daemon-reload && \
  systemctl --user enable rm-autoaim.service rm-bridge.service"

echo "Autostart service installed."
