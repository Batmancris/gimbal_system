# gimbal_system

This repository was restructured into a monorepo on 2026-03-19.

It is now the single source of truth for the local workspace and contains three primary code areas:

- `Gimbal control`: STM32 gimbal firmware
- `dev-branch`: ROS2 vision and bridge workspace
- `tianboard_s`: reference and backup board-level project

## Repository Layout

```text
gimbal_system/
├── Gimbal control/
├── dev-branch/
└── tianboard_s/
```

## Branch Policy

- `main`: stable primary branch
- `dev`: optional integration branch for ongoing development

Old multi-repo branches should be treated as historical references only, not as the long-term workflow.

## Notes

- This repository no longer uses the previous split daily-development model.
- Local migration backups and retired nested Git metadata are intentionally kept out of version control.
- Since 2026-03-19, the workspace has also completed a minimal USB CDC upper-to-lower control validation path.
- That validation remains an optional test path only and the default control chain still falls back to remote/UART behavior.
