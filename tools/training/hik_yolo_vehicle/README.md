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
