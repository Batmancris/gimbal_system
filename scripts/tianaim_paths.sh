#!/usr/bin/env bash

if [ -z "${BASH_VERSION:-}" ]; then
  echo "tianaim_paths.sh must be sourced from bash" >&2
  return 1 2>/dev/null || exit 1
fi

TIANAIM_REPO_ROOT="${TIANAIM_REPO_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"

tianaim_has_ros_packages() {
  local workspace="$1"
  [ -d "${workspace}/src" ] && find "${workspace}/src" -mindepth 2 -maxdepth 2 -name package.xml -print -quit | grep -q .
}

tianaim_resolve_ros_ws() {
  if [ -n "${TIANAIM_ROS_WS:-}" ]; then
    printf '%s\n' "${TIANAIM_ROS_WS}"
    return
  fi

  if tianaim_has_ros_packages "${TIANAIM_REPO_ROOT}/ros2_ws"; then
    printf '%s\n' "${TIANAIM_REPO_ROOT}/ros2_ws"
    return
  fi

  printf '%s\n' "${TIANAIM_REPO_ROOT}/ros2_ws"
}

tianaim_resolve_firmware_dir() {
  if [ -n "${TIANAIM_FIRMWARE_DIR:-}" ]; then
    printf '%s\n' "${TIANAIM_FIRMWARE_DIR}"
    return
  fi

  if [ -f "${TIANAIM_REPO_ROOT}/firmware/stm32_gimbal_control/Makefile" ]; then
    printf '%s\n' "${TIANAIM_REPO_ROOT}/firmware/stm32_gimbal_control"
    return
  fi

  printf '%s\n' "${TIANAIM_REPO_ROOT}/firmware/stm32_gimbal_control"
}

TIANAIM_ROS_WS="$(tianaim_resolve_ros_ws)"
TIANAIM_FIRMWARE_DIR="$(tianaim_resolve_firmware_dir)"
