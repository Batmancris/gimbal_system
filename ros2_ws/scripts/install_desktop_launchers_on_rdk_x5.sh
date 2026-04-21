#!/usr/bin/env bash
set -euo pipefail

RDK_USER="${RDK_USER:-sunrise}"
RDK_HOST="${RDK_HOST:?Please set RDK_HOST, e.g. export RDK_HOST=192.168.127.10}"
RDK_PORT="${RDK_PORT:-22}"
REMOTE_WS="${REMOTE_WS:-/home/sunrise/rm_ws}"
REMOTE_SRC_DIR="${REMOTE_SRC_DIR:-${REMOTE_WS}/src/ros2_ws}"
REMOTE_SCRIPT_DIR="${REMOTE_SCRIPT_DIR:-${REMOTE_SRC_DIR}/scripts}"
LOCAL_WS="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

echo "Syncing desktop launcher scripts to ${RDK_USER}@${RDK_HOST}:${REMOTE_SCRIPT_DIR}"

ssh -p "${RDK_PORT}" "${RDK_USER}@${RDK_HOST}" "mkdir -p '${REMOTE_SCRIPT_DIR}' ~/Desktop"

rsync -avz \
  "${LOCAL_WS}/scripts/desktop_start_full_stack.sh" \
  "${LOCAL_WS}/scripts/desktop_start_headless_stack.sh" \
  "${LOCAL_WS}/scripts/desktop_stop_full_stack.sh" \
  "${LOCAL_WS}/scripts/desktop_status_full_stack.sh" \
  "${RDK_USER}@${RDK_HOST}:${REMOTE_SCRIPT_DIR}/"

ssh -p "${RDK_PORT}" "${RDK_USER}@${RDK_HOST}" "\
  chmod +x '${REMOTE_SCRIPT_DIR}/desktop_start_full_stack.sh' \
           '${REMOTE_SCRIPT_DIR}/desktop_start_headless_stack.sh' \
           '${REMOTE_SCRIPT_DIR}/desktop_stop_full_stack.sh' \
           '${REMOTE_SCRIPT_DIR}/desktop_status_full_stack.sh' && \
  cat > ~/Desktop/start_autoaim.desktop <<'EOF'
[Desktop Entry]
Type=Application
Name=Start Autoaim
Comment=Start camera, inference, visualizer, and gimbal bridge
Exec=gnome-terminal -- /bin/bash -lc '${REMOTE_SCRIPT_DIR}/desktop_start_full_stack.sh; exec bash'
Terminal=false
EOF
  chmod +x ~/Desktop/start_autoaim.desktop && \
  cat > ~/Desktop/start_autoaim_headless.desktop <<'EOF'
[Desktop Entry]
Type=Application
Name=Start Autoaim Headless
Comment=Start camera, inference, and gimbal bridge without visualizer
Exec=gnome-terminal -- /bin/bash -lc '${REMOTE_SCRIPT_DIR}/desktop_start_headless_stack.sh; exec bash'
Terminal=false
EOF
  chmod +x ~/Desktop/start_autoaim_headless.desktop && \
  cat > ~/Desktop/stop_autoaim.desktop <<'EOF'
[Desktop Entry]
Type=Application
Name=Stop Autoaim
Comment=Stop camera, inference, visualizer, and gimbal bridge
Exec=gnome-terminal -- /bin/bash -lc '${REMOTE_SCRIPT_DIR}/desktop_stop_full_stack.sh; exec bash'
Terminal=false
EOF
  chmod +x ~/Desktop/stop_autoaim.desktop && \
  cat > ~/Desktop/autoaim_status.desktop <<'EOF'
[Desktop Entry]
Type=Application
Name=Autoaim Status
Comment=Show service, tmux, and process status
Exec=gnome-terminal -- /bin/bash -lc '${REMOTE_SCRIPT_DIR}/desktop_status_full_stack.sh; exec bash'
Terminal=false
EOF
  chmod +x ~/Desktop/autoaim_status.desktop"

echo "Desktop launchers installed."
