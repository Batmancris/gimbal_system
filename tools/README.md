# tools

[中文](#中文) | [English](#english)

## 中文

### 简介

`tools/` 保存与主线源码解耦的辅助工具、离线数据流程和诊断脚本。

### 当前结构

- `dbus_diag_reader.py`
  DBUS 诊断读取工具
- `capture/`
  数据采集工具骨架
- `labeling/`
  标注流程约定
- `training/`
  训练流程约定
- `evaluation/`
  评估流程约定

### 设计原则

- 工具应小而明确
- 不直接替代主线 ROS2 包和固件入口
- 新增工具优先使用无空格、全小写目录名
- 与数据、模型相关的工具应优先落在本目录，而不是塞回历史工作区

## English

`tools/` stores helper tools, offline workflows, and diagnostics that are decoupled from the runtime mainline.

Current areas:

- `dbus_diag_reader.py`
- `capture/`
- `labeling/`
- `training/`
- `evaluation/`

New tooling should stay focused, stable, and product-oriented.
