# HIK YOLO Vehicle Workflow

This directory contains the curated YOLOv8 vehicle-detection workflow migrated from the standalone local `yolo/` folder.

Included in Git:

- training, validation, and export entrypoint: `train_yolov8.py`
- test-set evaluation helper: `evaluate_vehicle_test.py`
- Hikrobot realtime detection UI: `realtime_detection_ui.py`
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
```

Ubuntu quick start:

```bash
bash tools/training/hik_yolo_vehicle/scripts/check_env.sh
bash tools/training/hik_yolo_vehicle/scripts/train.sh
```
