#!/usr/bin/env bash
set -euo pipefail

RDK_USER="${RDK_USER:-sunrise}"
RDK_HOST="${RDK_HOST:?Please set RDK_HOST, e.g. export RDK_HOST=192.168.1.10}"
RDK_PORT="${RDK_PORT:-22}"
REMOTE_WS="${REMOTE_WS:-/home/sunrise/rm_ws}"
LOCAL_WS="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

echo "Deploying workspace to ${RDK_USER}@${RDK_HOST}:${REMOTE_WS}"

ssh -p "${RDK_PORT}" "${RDK_USER}@${RDK_HOST}" "mkdir -p '${REMOTE_WS}/src'"

rsync -avz --delete \
  --exclude build \
  --exclude install \
  --exclude log \
  "${LOCAL_WS}/" "${RDK_USER}@${RDK_HOST}:${REMOTE_WS}/src/"

echo "Deploy finished."
