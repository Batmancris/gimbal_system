# Vehicle YOLOv8x 4090 Summary

Source: external vehicle detector training run. Raw datasets, checkpoints, and
intermediate artifacts are not tracked in this repository.

Known training summary from the original quick-start notes:

- best checkpoint: `runs/vehicle_yolov8x_4090/weights/best.pt`
- validation `mAP50`: about `0.995`
- validation `mAP50-95`: about `0.71184`
- best validation checkpoint appeared around epoch `7`

Repository policy:

- keep scripts, configs, and reports in Git
- keep raw datasets, `.rar` archives, generated predictions, and heavyweight checkpoints out of Git
- if a checkpoint must be tracked later, add it explicitly with approval and document its provenance
