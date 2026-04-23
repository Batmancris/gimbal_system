# HIK YOLO Vehicle Workflow

This directory contains the curated YOLOv8 vehicle-detection workflow migrated from the standalone local `yolo/` folder.

Included in Git:

- training, validation, and export entrypoint: `train_yolov8.py`
- test-set evaluation helper: `evaluate_vehicle_test.py`
- export consistency validator: `validate_pt_onnx_pipeline.py`
- quant-stage ONNX inspector: `inspect_onnx_stage_outputs.py`
- opset-11 compatibility fixer for controlled experiments: `fix_split_for_opset11.py`
- unified X5 quantization launcher: `run_x5_quant_pipeline.sh`
- sample X5 mapper config: `x5_quant_best_nv12.yaml`
- Hikrobot realtime detection UI: `realtime_detection_ui.py`
  - `--backend pt`: single PT preview
  - `--backend onnx`: single ONNX preview
  - `--backend compare`: PT and ONNX side-by-side compare
- Windows launcher scripts
- Linux bootstrap and training scripts
- training and dataset config files
- lightweight documentation and reports

Intentionally not committed:

- raw dataset archives such as `datasets.rar`
- training outputs under `runs/`
- local Ultralytics cache and temp files
- model weights such as `best.pt`, `last.pt`, and downloaded base checkpoints
- generated prediction images

Suggested repo mapping:

- dataset files live under `datasets/labeled/vehicle_detection_scene/`
- metadata and experiment summaries live under `models/reports/`
- code and launchers stay in this directory

Windows quick start:

```bat
set HIK_YOLO_PYTHON=E:\Anaconda\envs\hik_yolov8\python.exe
set HIK_MVS_ROOT=E:\MVS_Win_STD_4.6.3_260205\MVS\Development
tools\training\hik_yolo_vehicle\run_test_eval_windows.bat
tools\training\hik_yolo_vehicle\start_detection_ui_windows.bat
tools\training\hik_yolo_vehicle\start_detection_ui_windows.bat --backend onnx
tools\training\hik_yolo_vehicle\start_detection_ui_windows.bat --backend compare --device cpu
```

Ubuntu quick start:

```bash
bash tools/training/hik_yolo_vehicle/scripts/check_env.sh
bash tools/training/hik_yolo_vehicle/scripts/train.sh
```

Recommended export and validation flow:

```bash
python tools/training/hik_yolo_vehicle/train_yolov8.py --mode export --device cpu
python tools/training/hik_yolo_vehicle/validate_pt_onnx_pipeline.py \
  --pt /path/to/best.pt \
  --onnx /path/to/best.onnx \
  --image-dir /path/to/test/images
python tools/training/hik_yolo_vehicle/inspect_onnx_stage_outputs.py \
  --reference-onnx /path/to/best.onnx \
  --candidate-onnx /path/to/optimized_float_model.onnx /path/to/calibrated_model.onnx /path/to/quantized_model.onnx \
  --image-dir /path/to/test/images
```

## 2026-04-23 Vision PID Tuning Record

This repository state records the completed RDK X5 to STM32 C-board visual closed-loop tuning pass. The model files were not changed in this pass; the work focused on bridge latency, target safety, and lower-board vision PID behavior.

Final lower-board firmware parameters are in `firmware/stm32_gimbal_control/Src/gimbal_task.h`:

- `VISION_X_DEADBAND = 14.0f`, `VISION_Y_DEADBAND = 14.0f`
- `VISION_YAW_PID_KP = 0.0000072f`, `VISION_YAW_PID_KI = 0.0f`, `VISION_YAW_PID_KD = 0.000055f`
- `VISION_PITCH_PID_KP = 0.0000060f`, `VISION_PITCH_PID_KI = 0.0f`, `VISION_PITCH_PID_KD = 0.000042f`
- `VISION_MAX_ANGLE_STEP = 0.0045f`, `VISION_FAST_ANGLE_STEP = 0.0065f`, `VISION_FAST_ERROR_THRESHOLD = 160.0f`
- `VISION_CMD_SMOOTH_ALPHA = 0.42f`, `VISION_CMD_FAST_ALPHA = 0.58f`, `VISION_CMD_BRAKE_ALPHA = 0.92f`
- `VISION_SLOWDOWN_ERROR_PX = 220.0f`, `VISION_MIN_STEP_SCALE = 0.10f`
- `VISION_FRAME_HOLD_DECAY = 0.990f`, `VISION_FRAME_BRAKE_DECAY = 0.970f`
- `TARGET_STATE_SMOOTH_ALPHA = 1.00f` in `firmware/stm32_gimbal_control/Src/target_state.h`

Final behavior summary:

- The ROS bridge sends low-latency target centers and rejects unsafe target jumps instead of falling back to a different detection on the opposite side of the image.
- When target detection is lost or tracking continuity breaks, the bridge sends a neutral center frame so the lower board clears residual velocity instead of continuing to rotate blindly.
- The lower board uses frame-triggered vision PD updates with frame-to-frame command decay, braking alpha, and quadratic slowdown near image center. This keeps the fast-follow speed while reducing hard acceleration, overshoot, and stale-command drift.
- The current tested camera/detector path remains `hik_camera -> rm_vehicle_detection -> rm_gimbal_bridge -> STM32 USB-CDC` at roughly 30 FPS without visualization.
