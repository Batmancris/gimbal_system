# tianboard_s

[中文](#中文) | [English](#english)

## 中文

### 简介

`tianboard_s` 是当前工作区中的参考/备用板级工程。

它当前的定位不是系统主线，而是：

- 底层驱动参考工程
- USB Device 迁移参考来源
- 历史板级实现备份

### 当前定位

- 主线代码：否
- 当前整机联调入口：否
- 保留原因：参考、迁移、对照、必要时复用局部实现

### 适合在这里查什么

- USB Device 相关代码组织方式
- 某些底层外设初始化和板级工程结构
- 历史控制板实现差异

### 不建议怎么用

- 不要把这里当作当前唯一真实来源
- 不要在这里继续主线功能开发，除非明确要做参考提取或归档整理

## English

### Overview

`tianboard_s` is kept in the current workspace as a reference / backup board-level project.

It is not the active system mainline. Its role is:

- low-level driver reference
- USB Device migration reference source
- backup of historical board-level implementation

### Current Role

- Mainline code: no
- Current full-system integration entry: no
- Reason to keep it: reference, migration, comparison, and selective reuse

### What It Is Useful For

- USB Device code organization reference
- examples of peripheral initialization and board-level project structure
- comparing historical controller implementations

### What Not To Do

- do not treat this directory as the current single source of truth
- do not continue mainline feature development here unless the goal is explicit reference extraction or archival cleanup
