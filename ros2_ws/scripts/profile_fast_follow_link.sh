#!/usr/bin/env bash
set -eo pipefail

# profile_fast_follow_link.sh — fast_best profile health check
# Deploy to: /home/sunrise/rm_ws/scripts/profile_fast_follow_link.sh

REMOTE_WS="${REMOTE_WS:-/home/sunrise/rm_ws}"
BEAR_TOPIC="${BEAR_TOPIC:-/bear_detection/targets}"

cd "${REMOTE_WS}"
source /opt/tros/humble/setup.bash
source install/setup.bash 2>/dev/null || true

echo "========================================"
echo "  fast_best profile health check"
echo "========================================"
echo ""

# ---- 1. tmux session status ----
echo "--- tmux sessions ---"
tmux list-sessions 2>/dev/null || echo "(no tmux sessions)"
echo ""

# ---- 2. topic list ----
echo "--- ros2 topic list ---"
ros2 topic list
echo ""

# ---- 3. /hbmem_img Hz ----
echo "--- /hbmem_img Hz (timeout 10, window 30) ---"
hbmem_raw=$(timeout 10 ros2 topic hz /hbmem_img --window 30 2>&1 || true)
echo "$hbmem_raw"

# Extract the LAST "average rate:" line (avoid multi-line concatenation)
hbmem_avg_hz=$(echo "$hbmem_raw" | grep -E '^average rate:' | tail -1 | sed 's/.*average rate: *\([0-9.]*\).*/\1/')
# Extract the LAST "min: ... max: ... std dev:" line
hbmem_stats=$(echo "$hbmem_raw" | grep -E '^\s+min:' | tail -1)
hbmem_min_interval=$(echo "$hbmem_stats" | sed 's/.*min: *\([0-9.]*\)s.*/\1/')
hbmem_max_interval=$(echo "$hbmem_stats" | sed 's/.*max: *\([0-9.]*\)s.*/\1/')
hbmem_stddev=$(echo "$hbmem_stats" | sed 's/.*std dev: *\([0-9.]*\)s.*/\1/')

echo ""
echo "[parsed] hbmem_avg_hz=${hbmem_avg_hz:-N/A}"
echo "[parsed] hbmem_min_interval=${hbmem_min_interval:-N/A}"
echo "[parsed] hbmem_max_interval=${hbmem_max_interval:-N/A}"
echo "[parsed] hbmem_stddev=${hbmem_stddev:-N/A}"

if [ -z "$hbmem_avg_hz" ] || [ "$hbmem_avg_hz" = "N/A" ]; then
  echo "RESULT: /hbmem_img — NO DATA"
elif (( $(echo "$hbmem_avg_hz < 20" | bc -l 2>/dev/null || echo 0) )); then
  echo "RESULT: /hbmem_img — LOW (${hbmem_avg_hz} Hz)"
else
  echo "RESULT: /hbmem_img — OK (${hbmem_avg_hz} Hz)"
fi

if [ -z "$hbmem_max_interval" ] || [ "$hbmem_max_interval" = "N/A" ]; then
  echo "WARNING: hbmem max_interval could not be parsed — NEEDS_CHECK"
elif (( $(echo "$hbmem_max_interval > 0.06" | bc -l 2>/dev/null || echo 0) )); then
  echo "WARNING: hbmem max_interval=${hbmem_max_interval}s > 60ms — possible jitter"
else
  echo "hbmem max_interval=${hbmem_max_interval}s — OK"
fi
echo ""

# ---- 4. /bear_detection/targets Hz ----
echo "--- ${BEAR_TOPIC} Hz (timeout 10, window 50) ---"
bear_raw=$(timeout 10 ros2 topic hz "${BEAR_TOPIC}" --window 50 2>&1 || true)
echo "$bear_raw"

bear_avg_hz=$(echo "$bear_raw" | grep -E '^average rate:' | tail -1 | sed 's/.*average rate: *\([0-9.]*\).*/\1/')
bear_stats=$(echo "$bear_raw" | grep -E '^\s+min:' | tail -1)
bear_max_interval=$(echo "$bear_stats" | sed 's/.*max: *\([0-9.]*\)s.*/\1/')

echo ""
echo "[parsed] bear_avg_hz=${bear_avg_hz:-N/A}"
echo "[parsed] bear_max_interval=${bear_max_interval:-N/A}"

if [ -z "$bear_avg_hz" ] || [ "$bear_avg_hz" = "N/A" ]; then
  echo "RESULT: ${BEAR_TOPIC} — NO DATA"
elif (( $(echo "$bear_avg_hz < 20" | bc -l 2>/dev/null || echo 0) )); then
  echo "RESULT: ${BEAR_TOPIC} — LOW (${bear_avg_hz} Hz)"
else
  echo "RESULT: ${BEAR_TOPIC} — OK (${bear_avg_hz} Hz)"
fi

if [ -z "$bear_max_interval" ] || [ "$bear_max_interval" = "N/A" ]; then
  echo "WARNING: bear max_interval could not be parsed — NEEDS_CHECK"
elif (( $(echo "$bear_max_interval > 0.06" | bc -l 2>/dev/null || echo 0) )); then
  echo "WARNING: bear max_interval=${bear_max_interval}s > 60ms — possible jitter"
else
  echo "bear max_interval=${bear_max_interval}s — OK"
fi
echo ""

# ---- 5. topic info ----
echo "--- /hbmem_img topic info ---"
ros2 topic info /hbmem_img -v 2>/dev/null | head -20 || ros2 topic info /hbmem_img
echo ""

echo "--- ${BEAR_TOPIC} topic info ---"
ros2 topic info "${BEAR_TOPIC}" -v 2>/dev/null | head -20 || ros2 topic info "${BEAR_TOPIC}"
echo ""

# ---- 6. image resolution check ----
echo "--- /hbmem_img resolution (one-shot echo) ---"
res_raw=$(timeout 5 ros2 topic echo /hbmem_img --once 2>&1 || true)
msg_width=$(echo "$res_raw" | grep -E '^\s*width:' | head -1 | sed 's/.*width: *\([0-9]*\).*/\1/')
msg_height=$(echo "$res_raw" | grep -E '^\s*height:' | head -1 | sed 's/.*height: *\([0-9]*\).*/\1/')
echo "[parsed] msg width=${msg_width:-N/A} height=${msg_height:-N/A}"

# Compare with camera_info.yaml
ci_width=$(grep 'image_width:' "${REMOTE_WS}/src/hik_camera/config/camera_info.yaml" 2>/dev/null | awk '{print $2}' || echo "N/A")
ci_height=$(grep 'image_height:' "${REMOTE_WS}/src/hik_camera/config/camera_info.yaml" 2>/dev/null | awk '{print $2}' || echo "N/A")
echo "[camera_info.yaml] width=${ci_width:-N/A} height=${ci_height:-N/A}"

if [ -n "$msg_width" ] && [ -n "$ci_width" ] && [ "$msg_width" != "$ci_width" ]; then
  echo "WARNING: message width (${msg_width}) != camera_info width (${ci_width})"
fi
echo ""

# ---- 7. CPU usage ----
echo "--- CPU usage (top snapshot) ---"
hik_cpu=$(ps -C hik_camera_node -o %cpu= 2>/dev/null | head -1 | tr -d ' ' || echo "N/A")
bear_cpu=$(ps -C rm_bear_detection -o %cpu= 2>/dev/null | head -1 | tr -d ' ' || echo "N/A")
echo "hik_camera_node CPU: ${hik_cpu:-N/A}%"
echo "rm_bear_detection CPU: ${bear_cpu:-N/A}%"
echo ""

# ---- 8. Summary ----
echo "========================================"
echo "  Summary"
echo "========================================"
echo "hbmem_img:      avg=${hbmem_avg_hz:-N/A} Hz  max_interval=${hbmem_max_interval:-N/A}s"
echo "bear_targets:   avg=${bear_avg_hz:-N/A} Hz  max_interval=${bear_max_interval:-N/A}s"
echo "resolution:     msg=${msg_width:-?}x${msg_height:-?}  camera_info=${ci_width:-?}x${ci_height:-?}"
echo "CPU:            hik=${hik_cpu:-N/A}%  bear=${bear_cpu:-N/A}%"
echo ""
echo "Done."
