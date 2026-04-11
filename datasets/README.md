# datasets

[中文](#中文) | [English](#english)

## 中文

### 目录约定

本目录用于 TianAim 未来的数据闭环，不直接替代当前运行代码。

当前骨架：

```text
datasets/
├── raw/         # 原始采集图像
├── labeled/     # 标注后的数据
├── splits/      # train/val/test 切分
└── manifests/   # session manifest 与元数据
```

### 命名建议

建议图片命名格式：

```text
<session_id>_<camera_id>_<timestamp_ns>_<frame_index>.jpg
```

### 注意事项

- 不要默认把大体量原始数据直接提交到 Git
- manifest 和小体量样例可以保留在仓库中
- 真实数据建议通过对象存储或专用数据盘管理

## English

This directory is the product-facing dataset root for TianAim.

Suggested image naming:

```text
<session_id>_<camera_id>_<timestamp_ns>_<frame_index>.jpg
```

Keep metadata and small examples in Git. Large raw datasets should usually stay outside the repository.
