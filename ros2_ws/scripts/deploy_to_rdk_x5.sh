#!/usr/bin/env bash
set -euo pipefail

RDK_USER="${RDK_USER:-sunrise}"
RDK_HOST="${RDK_HOST:?Please set RDK_HOST, e.g. export RDK_HOST=192.168.1.10}"
RDK_PORT="${RDK_PORT:-22}"
REMOTE_WS="${REMOTE_WS:-/home/sunrise/rm_ws}"
REMOTE_SRC_DIR="${REMOTE_SRC_DIR:-${REMOTE_WS}/src}"
LOCAL_WS="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
if find "${LOCAL_WS}/src" -mindepth 2 -maxdepth 2 -name package.xml -print -quit 2>/dev/null | grep -q .; then
  LOCAL_SRC_DIR="${LOCAL_SRC_DIR:-${LOCAL_WS}/src}"
else
  LOCAL_SRC_DIR="${LOCAL_SRC_DIR:-${LOCAL_WS}}"
fi
LOCAL_SCRIPT_DIR="${LOCAL_SCRIPT_DIR:-${LOCAL_WS}/scripts}"
REMOTE_SCRIPT_DIR="${REMOTE_SCRIPT_DIR:-${REMOTE_SRC_DIR}/scripts}"

echo "Deploying ROS2 packages from ${LOCAL_SRC_DIR} to ${RDK_USER}@${RDK_HOST}:${REMOTE_SRC_DIR}"

ssh -p "${RDK_PORT}" "${RDK_USER}@${RDK_HOST}" "mkdir -p '${REMOTE_SRC_DIR}' '${REMOTE_SCRIPT_DIR}'"

rsync -avz --delete \
  --exclude build \
  --exclude install \
  --exclude log \
  "${LOCAL_SRC_DIR}/" "${RDK_USER}@${RDK_HOST}:${REMOTE_SRC_DIR}/"

if [ -d "${LOCAL_SCRIPT_DIR}" ]; then
  echo "Deploying scripts from ${LOCAL_SCRIPT_DIR} to ${RDK_USER}@${RDK_HOST}:${REMOTE_SCRIPT_DIR}"
  rsync -avz \
    "${LOCAL_SCRIPT_DIR}/" "${RDK_USER}@${RDK_HOST}:${REMOTE_SCRIPT_DIR}/"
fi

echo "Deploy finished."
