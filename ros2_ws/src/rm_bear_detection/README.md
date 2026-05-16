# rm_bear_detection

`rm_bear_detection` 是当前主线使用的单类别 YOLOv8 bear 检测节点。它订阅 RDK X5 的 NV12 shared-memory 图像，发布 `ai_msgs/msg/PerceptionTargets`，给 `rm_gimbal_bridge` 做云台跟随。

## 当前状态

- 当前默认目标类型：`bear`
- 当前默认输出话题：`/bear_detection/targets`
- 低速跟随已验证顺滑。
- 高速跟随仍然会跟不上，需要结合检测稳定性、桥接预测和下位机角度增量继续调试。

当前主线：

```text
/hbmem_img -> rm_bear_detection -> /bear_detection/targets -> rm_gimbal_bridge
```

## 模型

默认安装路径：

```bash
/home/sunrise/rm_ws/install/rm_bear_detection/lib/rm_bear_detection/config/bear_yolov8n_x5_640_nv12.bin
```

源码目录内模型：

```bash
ros2_ws/src/rm_bear_detection/config/bear_yolov8n_x5_640_nv12.bin
```

## 关键默认参数

```bash
target_type=bear
box_format=cxcywh
score_threshold=0.71
nms_threshold=0.70
nms_top_k=300
stable_required_hits=2
stable_match_radius_px=140.0
stable_max_track_age_ms=200
task_num=4
publish_debug_text=false
log_detections=false
```

## 运行

```bash
cd /home/sunrise/rm_ws
source /opt/tros/humble/setup.bash
source install/setup.bash

ros2 run rm_bear_detection rm_bear_detection_node --ros-args \
  -p output_topic:=/bear_detection/targets
```

调试日志：

```bash
ros2 run rm_bear_detection rm_bear_detection_node --ros-args \
  -p output_topic:=/bear_detection/targets \
  -p publish_debug_text:=true \
  -p log_detections:=true
```

## 处理流程

1. 读取 `/hbmem_img` 的 NV12 图像。
2. 如果图像尺寸和模型输入不一致，做 letterbox，Y 平面填 114，UV 平面填 128。
3. 调用 RDK X5 DNN 推理。
4. 按 `cxcywh` 解析 YOLOv8 输出。
5. 把检测框映射回原图坐标。
6. 用 `stable_required_hits` 做短时稳定筛选，减少偶发误检直接进入云台控制。
7. 发布 `ai_msgs/msg/PerceptionTargets`。

## 高速问题相关说明

高速跟不上不一定是检测节点本身的问题，但检测节点需要重点确认：

- 高速运动时是否还能连续检测到 bear。
- `stable_required_hits=2` 是否带来可接受的延迟。
- `score_threshold=0.71` 是否会在运动模糊时丢目标。
- 输出框中心是否稳定，是否存在明显跳变。

如果高速时目标短暂丢失，可以临时降低阈值或稳定命中数做对比测试。当前 `score_threshold` 和 `stable_required_hits` 均通过 ROS2 parameters（`declare_parameter`）配置，不使用环境变量 `BEAR_SCORE_THRESHOLD` / `BEAR_STABLE_REQUIRED_HITS`。

诊断时可通过 `--ros-args` 临时覆盖参数，例如：

```bash
ros2 run rm_bear_detection rm_bear_detection_node --ros-args \
  -p score_threshold:=0.65 \
  -p stable_required_hits:=1
```

这只是诊断手段，不代表最终参数。

