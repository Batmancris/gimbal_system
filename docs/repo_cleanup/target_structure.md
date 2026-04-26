# Target Structure

Updated: 2026-04-26

训练/量化/采集工具已拆分为独立工具仓，本仓库仅保留云台主系统代码。

## Final Main Repository Layout

```text
gimbal_system/
├── README.md
├── AGENTS.md
├── ros2_ws/
├── firmware/
├── scripts/
├── models/
├── datasets/
├── docs/
├── assets/
└── archive/
```

## Directory Roles

- `README.md`: public entry point.
- `AGENTS.md`: AI agent collaboration and repository governance rules.
- `ros2_ws/`: ROS2/TROS upper-level runtime workspace.
- `firmware/`: STM32 lower-level firmware.
- `scripts/`: top-level build and run wrappers.
- `models/`: model metadata, reports, and runtime configuration notes.
- `datasets/`: dataset skeleton, manifests, and examples only.
- `docs/`: architecture, cleanup, release, and audit documents.
- `assets/`: lightweight visual assets.
- `archive/`: historical audit and recovery notes.

## Excluded From Main Repository

- Training tools.
- Quantization tools.
- Capture tools.
- Raw datasets and videos.
- Large training weights and intermediate artifacts.
- Local caches, credentials, and machine-specific configuration.
