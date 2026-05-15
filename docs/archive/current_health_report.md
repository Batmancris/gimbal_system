# TianAim 当前体检报告

> 生成时间: 2026-05-15
> 基线 commit: f8cdec2
> 分支: feature/formula-mini-kt-cloud-follow（历史命名；当前运行目标是 bear，KT 适配是后续工作）

## 1. 当前稳定基线

| 项目 | 值 |
|---|---|
| GitHub branch | `feature/formula-mini-kt-cloud-follow`（历史命名；当前运行目标是 bear） |
| HEAD commit | `f8cdec2 chore: consolidate headless fast follow startup scripts` |
| 推荐启动命令 | `ssh rdk-x5 "bash /home/sunrise/rm_ws/scripts/start_fast_follow_verified.sh"` |
| 回滚启动命令 | `ssh rdk-x5 "FOLLOW_PROFILE=stable bash /home/sunrise/rm_ws/scripts/start_fast_follow_verified.sh"` |
| 当前稳定链路 | `hik_camera -> /hbmem_img -> rm_bear_detection -> /bear_detection/targets -> rm_gimbal_bridge -> /dev/ttyACM0 -> STM32 -> 云台` |

### 当前禁止事项

- 不使用 `/image_raw` 作为主链路依赖
- 不启用 `rm_vis` 可视化节点
- 不启用 `rm_armor_detection_visualizer`
- 不使用 `publish_image_raw:=true`
- 不使用 `single_target_hold_ms` / `SelectSingleTarget`（已清除历史坏改动）
- 不新建 `start_xxx_new.sh` 启动脚本
- 不修改 bridge 参数、fast_best 参数、电机 PID、YOLO 模型、检测逻辑

## 2. 开机状态

用户已验证的 clean boot 目标：

- 开机后无 ROS 节点运行
- 无 tmux 会话残留
- 无 hik_camera / rm_bear_detection / rm_gimbal_bridge 进程
- 无 rm_vis / rm_armor_detection_visualizer 进程
- 无 apt-show-versions / apt-check / apt-helper 后台任务
- 只通过一条命令启动完整云台链路

## 3. 启动链路

### 唯一推荐启动

```bash
ssh rdk-x5 "bash /home/sunrise/rm_ws/scripts/start_fast_follow_verified.sh"
```

### 启动成功判据

启动脚本输出中应包含以下四行：

```text
Camera OK.
Detection OK.
Bridge OK.
FAST FOLLOW READY.
profile: fast_best.
```

### 启动失败排查

如果缺少任一 OK，检查对应 tmux 日志：

```bash
ssh rdk-x5 "tmux -L autoaim capture-pane -pt hik_cam"
ssh rdk-x5 "tmux -L autoaim capture-pane -pt rm_det"
ssh rdk-x5 "tmux -L autoaim capture-pane -pt rm_bridge"
```

## 4. ROS 链路验收

### 验收命令

```bash
ssh rdk-x5 "source /opt/tros/humble/setup.bash; \
  source /home/sunrise/rm_ws/install/setup.bash; \
  echo ---nodes---; ros2 node list; \
  echo ---hbmem---; ros2 topic info /hbmem_img; \
  echo ---targets---; ros2 topic info /bear_detection/targets; \
  echo ---serial---; fuser -v /dev/ttyACM0 2>/dev/null || true; \
  echo ---tmux---; tmux -L autoaim ls 2>/dev/null || true; \
  echo ---vis---; ps aux | grep -E 'rm_vis|armor_detection_visualizer|publish_image_raw' | grep -v grep || echo no_vis"
```

### 正确结果

| 检查项 | 预期值 |
|---|---|
| ros2 node list | `/hik_camera`, `/rm_bear_detection`, `/rm_gimbal_bridge` |
| /hbmem_img | 1 publisher, 1 subscriber |
| /bear_detection/targets | 1 publisher, 1 subscriber |
| /dev/ttyACM0 | 被 rm_gimbal_bridge 占用 |
| tmux -L autoaim | hik_cam, rm_det, rm_bridge 三个会话 |
| rm_vis / visualizer | 无相关进程 |

### 2026-05-15 实测结果

| 检查项 | 实测值 |
|---|---|
| ros2 node list | `/hik_camera`, `/rm_bear_detection`, `/rm_gimbal_bridge` |
| /hbmem_img | 1 pub / 1 sub |
| /bear_detection/targets | 1 pub / 1 sub |
| /dev/ttyACM0 | PID 3938 = rm_gimbal_bridge_node |
| tmux sessions | hik_cam, rm_bridge, rm_det |
| bad processes | clean |

## 5. 当前性能与体感结论

### 实测体感

- 当前实测跟随丝滑，卡顿明显减少
- 低速跟随已验证稳定

### 偶发卡顿分析

剩余偶发卡顿主要怀疑来自：
- 目标快速出框导致短暂空帧
- 边缘检测 bbox 突变
- 相机/检测端偶发高延迟帧

当前不再优先怀疑：启动脚本问题、rm_vis 干扰、apt 后台任务干扰、bridge 链路问题。

### Profile 数据 (2026-05-15 实时采集)

```text
/hbmem_img:      avg=29.622 Hz  max_interval=0.092s  std_dev=0.01973s
/bear_detection/targets:  avg=37.762 Hz  max_interval=0.087s  std_dev=0.01751s
resolution:      1280x1024
CPU:             hik_camera=128%  rm_bear_detection=307%
```

- `/hbmem_img` 平均 ~30 Hz，偶发 max_interval 92ms (>60ms 阈值)，存在轻微抖动
- `/bear_detection/targets` 平均 ~38 Hz，偶发 max_interval 87ms
- 相机 CPU 128%，检测 CPU 307%（YOLO 推理预期负载）
- 两条 topic 均有 max_interval > 60ms 的 WARNING，但未导致跟随中断

## 6. 当前已解决问题

按项目历史整理：

1. **统一真实链路**：确认主链路为 `/hbmem_img -> /bear_detection/targets -> bridge`，排除 `/image_raw` 干扰
2. **排除 rm_vis 干扰**：rm_vis / rm_armor_detection_visualizer 不用于 headless 主链路
3. **清除历史坏改动**：已移除 `single_target_hold_ms` / `SelectSingleTarget` 相关代码
4. **固定 fast_best 启动入口**：`start_fast_follow_verified.sh` 为唯一推荐启动脚本
5. **关闭自动 apt 后台任务**：清理 apt-show-versions / apt-check / apt-helper
6. **清理开机自启动**：做到 clean boot，无残留 tmux / 进程 / service
7. **同步关键脚本并提交到 GitHub**：f8cdec2 已推送
8. **确认一条命令启动成功**：从 Windows SSH 一键启动完整链路

## 7. 当前剩余风险

| 风险 | 严重度 | 说明 |
|---|---|---|
| 目标快速出框 | 中 | 导致短暂空帧，跟随中断 |
| 检测多框/bbox 跳变 | 中 | 仍需后续专项分析 |
| 相机/检测 CPU 较高 | 低 | hik 128% + bear 307%，尾延迟仍需后续 profile |
| 历史脚本较多 | 低 | 启动脚本虽已收口，但历史脚本仍保留在仓库中 |
| README/docs 需统一 | 低 | 部分文档仍引用旧链路或旧参数 |
