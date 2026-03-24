[English](./README.md) | 简体中文

# rm_armor_detection

## 功能定位

`rm_armor_detection` 是当前项目在 RDK-X5 / TROS 环境下使用的 YOLOv8 装甲板检测主线包。

它承担三件核心事情：

- 订阅 `/hbmem_img`，在 BPU 上执行推理
- 解析装甲板检测框、关键点和类别，并发布到 `/dnn_node_sample`
- 提供 `rm_armor_detection_visualizer`，把实时检测结果画回相机图像并在 RDK-X5 屏幕显示

## 当前主线输入输出

- 推理输入：`/hbmem_img`
- 检测输出：`/dnn_node_sample`
- 可视化输入：`/image_raw` + `/dnn_node_sample`

## 当前联调说明

- 当前项目联调更偏向较暗、高对比度画面，以突出 RM 装甲板灯条
- 当前仓库内检测分数阈值已经放宽，便于整链路 bring-up 和排查
- 桌面可视化窗口用于确认“实时相机画面”和“检测结果”是否一致，而不是替代整机桥接链路

## 编译

```bash
source /opt/tros/humble/setup.bash
colcon build --packages-select rm_armor_detection
```

## 运行检测节点

```bash
ros2 launch rm_armor_detection rm_armor_detection.launch.py
```

如果需要网页展示：

```bash
export WEB_SHOW=TRUE
ros2 launch rm_armor_detection rm_armor_detection.launch.py
```

## 运行桌面可视化窗口

在 RDK-X5 图形桌面环境中执行：

```bash
source /opt/tros/humble/setup.bash
source install/setup.bash
export DISPLAY=:0
export XAUTHORITY=/home/sunrise/.Xauthority
ros2 run rm_armor_detection rm_armor_detection_visualizer
```

窗口功能：

- 订阅 `/image_raw`
- 订阅 `/dnn_node_sample`
- 绘制检测框、类别、置信度和关键点

## 说明

- 当前项目主线关注的是“检测结果接桥接链路再下发给下位机”
- 桌面可视化节点现在已经纳入 `start_autoaim_tmux.sh` 的默认板端三节点启动链
- 如果后续要进一步固化部署，可以再把“板端可视化链”和“桥接下位机链”整理成显式可选的 launch 或 service 组合
