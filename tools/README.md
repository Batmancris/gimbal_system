# tools

[中文](#中文) | [English](#english)

## 中文

### 简介

`tools` 目录保存与主线源码解耦的辅助工具和诊断脚本。

### 当前内容

- `dbus_diag_reader.py`
  用于读取或排查与 DBUS 相关的诊断信息

### 当前定位

- 主线源码：否
- 使用方式：按需单独调用
- 适合用途：排障、观测、辅助验证

### 说明

这里的工具脚本应尽量保持“小而明确”，不要替代主线包内的正式运行入口。

## English

### Overview

The `tools` directory stores helper tools and diagnostics that are decoupled from the mainline source trees.

### Current Contents

- `dbus_diag_reader.py`
  used for DBUS-related reading or diagnostic inspection

### Current Role

- mainline source code: no
- usage mode: run only when needed
- best used for: debugging, observation, and auxiliary validation

### Note

Scripts in this directory should stay small and focused, and should not replace the formal runtime entry points of the mainline packages.
