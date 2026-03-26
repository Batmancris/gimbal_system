# _git_migration_backup

[中文](#中文) | [English](#english)

## 中文

### 简介

`_git_migration_backup` 用于保存仓库归并和迁移时的备份产物。

### 当前定位

- 当前主线开发目录：否
- 保留原因：回溯、审计、兜底恢复
- 推荐操作：只读查看，不在此处继续开发

### 当前内容

- 仓库打包备份
- 分支状态快照
- 远端信息记录
- 迁移差异补丁
- 归并清单

### 说明

如果后续还要保留迁移备份，建议统一继续放在这里，而不要把 `.git.BAK-*` 分散留在各主目录里。

## English

### Overview

`_git_migration_backup` stores repository-consolidation and migration backup artifacts.

### Current Role

- active development path: no
- reason to keep: traceability, audit, and recovery fallback
- recommended usage: read-only inspection

### Current Contents

- archived repository backups
- branch-state snapshots
- remote metadata records
- migration diff patches
- consolidation manifest files

### Note

If more migration backups need to be retained in the future, keep them centralized here instead of leaving `.git.BAK-*` directories scattered across the main project folders.
