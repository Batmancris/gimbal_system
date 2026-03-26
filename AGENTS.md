# AGENTS Guide

This file is the shared repository guide for AI coding agents working in this project.

It is intentionally model-agnostic. Model-specific guidance files such as `CLAUDE.md` or other agent notes should extend this file instead of duplicating it.

## Project Role

This repository is the current top-level entry for the gimbal system project.

It contains:

- the active upper-level ROS2 / TROS / RDK-X5 workspace
- the active lower-level STM32 firmware mainline
- retained historical and reference projects
- deployment, integration, and migration support material

## Source Of Truth

When documents conflict, use this order:

1. current source files, launch files, scripts, and service files
2. the `README.md` in the relevant directory
3. historical handover notes and retained legacy documents

Do not treat older `Readme.md`, `Readme_zh.md`, or `README_cn.md` files as primary sources unless they explicitly act as redirect pages.

## Mainline Paths

The current recommended whole-system path is:

```text
hik_camera
  -> rm_armor_detection
  -> rm_gimbal_bridge
  -> UART
  -> Gimbal control
```

Current repository conclusions:

- `dev-branch/` is the formal upper-level mainline
- `Gimbal control/` is the formal lower-level mainline
- `UART` is still the default stable whole-system communication path
- `USB CDC` is a migration-validation path and must not be assumed to have fully replaced UART
- `tianboard_s/` is reference-only and is not the place for ongoing mainline feature work

## Directory Trust Levels

### Active Mainline

- `dev-branch/`
- `Gimbal control/`

These are the first places to read and the default places to make mainline changes.

### Shared Support

- `tools/`
- `dev-branch/scripts/`
- `dev-branch/rm_interfaces/`
- `dev-branch/rm_utils/`

These support the active mainline but are usually not the top-level runtime entry by themselves.

### Reference Or Historical

- `tianboard_s/`
- `dev-branch/armor_detector/`
- `dev-branch/rm_camera_driver/`
- `dev-branch/rm_camera_driver_nv12/`
- `dev-branch/work_handover/`
- `dev-branch/ultralytics-8.2.103/`
- `_git_migration_backup/`

These directories may be useful for reference, comparison, or recovery context, but they should not be mistaken for the current default development path.

## Recommended Reading Order For Agents

When starting a new task, read in this order unless the user gives a more specific scope:

1. `README.md`
2. `dev-branch/README.md`
3. `dev-branch/scripts/README.md`
4. `Gimbal control/README.md`
5. the package-level `README.md` closest to the files you will touch

## Common Mistakes To Avoid

- Do not assume the old split-repository name `rm_armor_tracker` is still the active project structure.
- Do not treat `USB CDC` as the default runtime communication path unless the user explicitly says the migration is complete.
- Do not continue feature development in `tianboard_s/` unless the task is specifically about reference extraction or archival cleanup.
- Do not treat historical screenshots, handover notes, or migration backups as the primary source of truth.
- Do not assume a retained third-party source snapshot such as `ultralytics-8.2.103/` is part of the active runtime chain.

## Documentation Expectations

When editing or adding documentation:

- prefer bilingual `README.md` files for primary project-facing docs
- keep older language-split files as redirect pages instead of full independent documents when possible
- describe current role, current boundaries, reading guidance, and mainline relationship clearly
- avoid reviving outdated commands, old repository names, or stale package relationships

## Change Scope Expectations

Unless the user explicitly asks otherwise:

- prefer minimal, targeted changes
- do not rewrite unrelated historical material
- do not delete retained reference directories only because they are not mainline
- do not change source code when the task is documentation-only

## Git And Submission Expectations

When making changes:

- keep documentation-only changes in documentation-only commits when practical
- avoid mixing repository cleanup guidance with unrelated code edits
- preserve user changes you did not create
- do not rewrite history unless the user explicitly requests it

## If You Need A Model-Specific Layer

If a future agent-specific file is added, such as `CLAUDE.md`, it should usually contain:

- output style preferences for that model
- review or planning habits specific to that model
- any extra reminders about how that model should consume `AGENTS.md`

It should not duplicate the repository facts already written here unless duplication is truly necessary.
