# Tools Split Note

Updated: 2026-04-26

## Decision

`tools/` 已从 `gimbal_system` 云台主仓库拆出。

当前仓库只保留云台主代码，包括 ROS2 上位机、STM32 下位机、部署入口、模型配置、数据集骨架和文档。

## External Tooling

后续训练、量化、采集工具由独立工具仓维护，不再作为本仓库的组成部分。

这包括：

- 训练工具。
- 量化工具。
- 采集工具。
- 标注、评估、诊断等离线辅助工具。

## README Policy

README 中不再把 `tools/` 作为主仓组成部分。README 只说明：

训练/量化/采集工具已拆分为独立工具仓，本仓库仅保留云台主系统代码。

## Git Status Policy

本轮允许保留 Git 中对原 `tools/` 已跟踪文件的删除状态，不恢复这些文件，也不删除磁盘上的其他文件。
