This directory is the expected dataset root for the migrated YOLOv8 vehicle workflow.

Expected layout:

```text
datasets/labeled/vehicle_detection_scene/
├── images/
│   ├── train/
│   ├── val/
│   └── test/
└── labels/
    ├── train/
    ├── val/
    └── test/
```

Keep large image sets outside Git unless you intentionally add a tiny sample.
