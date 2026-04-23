# rm_vehicle_detection

`rm_vehicle_detection` 是 YOLOv8 vehicle 检测节点，目前保留为兼容和对照链路。当前默认主线已经切到 `rm_bear_detection`。

## 当前状态

- 默认输出话题：`/vehicle_detection/targets`
- 默认目标类型：`vehicle`
- 当前低速丝滑跟随主要在 bear 链路上验证。
- vehicle 链路仍可运行，但不是当前默认调试目标。

## 运行链路

```text
/hbmem_img -> rm_vehicle_detection -> /vehicle_detection/targets
```

如需接入桥接：

```text
/hbmem_img -> rm_vehicle_detection -> /vehicle_detection/targets -> rm_gimbal_bridge
```

## 关键默认参数

```bash
image_topic=/hbmem_img
output_topic=/vehicle_detection/targets
target_type=vehicle
box_format=xyxy
score_threshold=0.35
nms_threshold=0.5
nms_top_k=300
task_num=4
```

板端 `run_rm_det_loop.sh` 对 vehicle 模式会覆盖部分参数：

```bash
VEHICLE_BOX_FORMAT=cxcywh
VEHICLE_SCORE_THRESHOLD=0.10
```

## 运行

```bash
cd /home/sunrise/rm_ws
source /opt/tros/humble/setup.bash
source install/setup.bash

ros2 run rm_vehicle_detection rm_vehicle_detection_node --ros-args \
  -p output_topic:=/vehicle_detection/targets
```

通过脚本切到 vehicle：

```bash
export DETECTOR_TYPE=vehicle
export DETECTOR_TOPIC=/vehicle_detection/targets
export ALLOWED_TARGET_TYPES=vehicle
bash src/scripts/start_autoaim_tmux.sh
bash src/scripts/start_rm_bridge_tmux.sh
```

## 维护说明

如果后续 vehicle 模型导出格式、tensor 顺序或 head layout 变化，需要同步检查：

- `src/parser.cpp`
- `include/parser.h`
- `src/vehicle_detection_node.cpp`

当前高速跟随问题的主线调试不在 vehicle 节点上，除非现场明确切回 vehicle 场景。

