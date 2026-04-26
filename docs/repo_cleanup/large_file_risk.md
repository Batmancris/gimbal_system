# Large File Risk

Updated: 2026-04-26

## Current Tracked Large Files

No currently present tracked file is larger than 10 MB in this worktree.

Largest tracked files observed during this pass:

| Path | Size |
| --- | ---: |
| `ros2_ws/src/hik_camera/hikSDK/lib/amd64/libMvCameraControl.so` | 6,623,480 bytes |
| `ros2_ws/src/hik_camera/hikSDK/lib/amd64/libMvUsb3vTL.so` | 6,459,928 bytes |
| `ros2_ws/src/hik_camera/hikSDK/lib/arm64/libMvCameraControl.so` | 6,242,169 bytes |
| `ros2_ws/src/hik_camera/hikSDK/lib/arm64/libMvUsb3vTL.so` | 6,099,026 bytes |
| `ros2_ws/src/rm_bear_detection/config/bear_yolov8n_x5_640_nv12.bin` | 5,049,278 bytes |
| `ros2_ws/src/rm_vehicle_detection/config/quant.bin` | 4,721,596 bytes |
| `firmware/stm32_gimbal_control/algorithm/libarm_cortexM4lf_math.a` | 3,144,804 bytes |
| `ros2_ws/src/hik_camera/hikSDK/lib/amd64/libMediaProcess.so` | 2,885,520 bytes |
| `ros2_ws/src/hik_camera/hikSDK/lib/arm64/libMediaProcess.so` | 1,681,748 bytes |
| `firmware/stm32_gimbal_control/Drivers/CMSIS/Device/ST/STM32F4xx/Include/stm32f407xx.h` | 1,350,862 bytes |

## Tracked Binary Extensions

Tracked `.bin` files:

- `ros2_ws/src/rm_bear_detection/config/bear_yolov8n_x5_640_nv12.bin`
- `ros2_ws/src/rm_vehicle_detection/config/quant.bin`

Tracked `.so` files:

- Hikvision SDK shared libraries under `ros2_ws/src/hik_camera/hikSDK/lib/amd64/`
- Hikvision SDK shared libraries under `ros2_ws/src/hik_camera/hikSDK/lib/arm64/`

Tracked `.a` files:

- `firmware/stm32_gimbal_control/algorithm/libarm_cortexM4lf_math.a`

Tracked `.lib` files:

- `firmware/stm32_gimbal_control/algorithm/AHRS.lib`

## Git LFS Recommendation

Git LFS is not mandatory for the currently observed file sizes because no present tracked file exceeds 10 MB.

However, use one of these policies before public release:

- Preferred: replace vendor SDK binaries and model bins with download/setup instructions where practical.
- Acceptable: keep small runtime-critical binaries in Git only after license review.
- If larger model or SDK binaries must stay versioned later, use Git LFS or GitHub release assets instead of normal Git blobs.

## License Confirmation Needed

Confirm public redistribution permission for:

- Hikvision MVS SDK headers and shared libraries.
- STM32 HAL and CMSIS files.
- FreeRTOS middleware.
- ARM CMSIS DSP static library.
- `firmware/stm32_gimbal_control/algorithm/AHRS.lib`.
- Quantized model bins and any training data or labels used to produce them.

## Recommended Follow-Up

- Decide whether the two tracked quantized `.bin` files are runtime-critical for GitHub users.
- If they stay, document provenance, intended hardware, and checksum in `docs/model_registry.md`.
- If they move out, replace them with download instructions or release asset references.
- Add or confirm ignore rules for future `.pt`, `.onnx`, large `.bin`, video, dataset, and quantization output files.
