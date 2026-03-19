#!/usr/bin/env bash
set -euo pipefail

RDK_USER="${RDK_USER:-sunrise}"
RDK_HOST="${RDK_HOST:?Please set RDK_HOST, e.g. export RDK_HOST=192.168.127.10}"
RDK_PORT="${RDK_PORT:-22}"
REMOTE_WS="${REMOTE_WS:-/home/sunrise/rm_ws}"
LOCAL_WS="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

echo "Syncing autostart files to ${RDK_USER}@${RDK_HOST}:${REMOTE_WS}"

ssh -p "${RDK_PORT}" "${RDK_USER}@${RDK_HOST}" "mkdir -p '${REMOTE_WS}/src/scripts' ~/.config/systemd/user"

rsync -avz \
  "${LOCAL_WS}/scripts/clean_build_and_start_on_rdk.sh" \
  "${LOCAL_WS}/scripts/start_autoaim_tmux.sh" \
  "${LOCAL_WS}/scripts/check_autoaim_topics.sh" \
  "${LOCAL_WS}/scripts/rm-autoaim.service" \
  "${RDK_USER}@${RDK_HOST}:${REMOTE_WS}/src/scripts/"

ssh -p "${RDK_PORT}" "${RDK_USER}@${RDK_HOST}" "\
  chmod +x '${REMOTE_WS}/src/scripts/clean_build_and_start_on_rdk.sh' \
           '${REMOTE_WS}/src/scripts/start_autoaim_tmux.sh' \
           '${REMOTE_WS}/src/scripts/check_autoaim_topics.sh' && \
  cp '${REMOTE_WS}/src/scripts/rm-autoaim.service' ~/.config/systemd/user/rm-autoaim.service && \
  systemctl --user daemon-reload && \
  systemctl --user enable rm-autoaim.service"

echo "Autostart service installed."
