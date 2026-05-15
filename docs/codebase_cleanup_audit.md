# TianAim 代码收口审计

> 生成时间: 2026-05-15
> 基线 commit: f8cdec2
> 审计范围: 全仓文件分类，只做分析，不删除任何文件

## 1. 当前主链路文件

### ROS2 核心运行时（必须保留）

| 文件 | 作用 | 必需 | 说明 |
|---|---|---|---|
| `ros2_ws/src/hik_camera/` | 海康相机驱动节点 | 是 | 发布 /hbmem_img |
| `ros2_ws/src/rm_bear_detection/` | bear YOLO 检测节点 | 是 | 发布 /bear_detection/targets |
| `ros2_ws/src/rm_gimbal_bridge/` | 云台桥接节点 | 是 | 订阅 targets，发送 USB-CDC |
| `ros2_ws/src/rm_interfaces/` | ROS2 自定义消息/服务 | 是 | PerceptionTargets 等消息定义 |
| `ros2_ws/src/rm_utils/` | 通用工具库 | 是 | logger, math, heartbeat 等 |
| `ros2_ws/scripts/start_fast_follow_verified.sh` | 唯一推荐启动入口 | 是 | headless fast_best 链路 |
| `ros2_ws/scripts/run_hik_cam_loop.sh` | 相机节点循环守护 | 是 | tmux 底层 loop |
| `ros2_ws/scripts/run_rm_det_loop.sh` | 检测节点循环守护 | 是 | tmux 底层 loop |
| `ros2_ws/scripts/run_rm_bridge_loop.sh` | 桥接节点循环守护 | 是 | tmux 底层 loop，fast_best 参数在此维护 |

### 固件（必须保留）

| 文件 | 作用 | 必需 | 说明 |
|---|---|---|---|
| `firmware/stm32_gimbal_control/` | STM32F407 云台控制固件 | 是 | 视觉输入解析、电机 PID、CAN 控制 |

### 顶层构建/路径工具（必须保留）

| 文件 | 作用 | 必需 | 说明 |
|---|---|---|---|
| `scripts/build_ros2_mainline.sh` | ROS2 主线构建 | 是 | 本地构建入口 |
| `scripts/build_firmware_mainline.sh` | 固件构建 | 是 | 本地构建入口 |
| `scripts/tianaim_paths.sh` | 路径兼容变量 | 是 | 被其他脚本 source |

## 2. 启动脚本分类

| 脚本 | 类别 | 推荐状态 | 说明 |
|---|---|---|---|
| `start_fast_follow_verified.sh` | 唯一推荐入口 | **保留，主入口** | headless fast_best 一键启动 |
| `start_headless_follow_light.sh` | fallback 入口 | 保留，备用 | 被 verified 脚本调用 |
| `run_hik_cam_loop.sh` | 底层 loop | 保留 | 相机 tmux 守护 |
| `run_rm_det_loop.sh` | 底层 loop | 保留 | 检测 tmux 守护 |
| `run_rm_bridge_loop.sh` | 底层 loop | 保留 | 桥接 tmux 守护，fast_best 参数集中维护 |
| `profile_fast_follow_link.sh` | diagnostics | 保留 | 性能采集/健康检查 |
| `check_autoaim_topics.sh` | diagnostics | 保留 | 话题/tmux 状态检查 |
| `desktop_status_full_stack.sh` | diagnostics | 保留 | 全栈状态查看 |
| `desktop_stop_full_stack.sh` | diagnostics | 保留 | 全栈停止 |
| `usb_cdc_pitch_control_test.py` | diagnostics | 保留 | USB-CDC 协议测试工具 |
| `deploy_to_rdk_x5.sh` | deployment | 保留 | 从本机同步到板端 |
| `build_and_run_on_rdk_x5.sh` | deployment | 保留 | 远程构建启动 |
| `clean_build_and_start_on_rdk.sh` | deployment | 保留 | 清理重建启动 |
| `install_autostart_on_rdk_x5.sh` | service/autostart | 归档候选 | systemd 自动启动安装（当前不推荐使用） |
| `install_desktop_launchers_on_rdk_x5.sh` | service/autostart | 归档候选 | 桌面快捷方式安装 |
| `rm-autoaim.service` | service/autostart | 归档候选 | systemd service 文件 |
| `rm-bridge.service` | service/autostart | 归档候选 | systemd service 文件 |
| `start_autoaim_tmux.sh` | legacy | 归档候选 | 旧版手动 tmux 启动，含 visualizer |
| `start_rm_bridge_tmux.sh` | legacy | 归档候选 | 旧版手动桥接 tmux 启动 |
| `start_rm_vis_tmux.sh` | visualizer | 归档候选 | rm_vis 可视化启动，不用于主链路 |
| `run_rm_vis_loop.sh` | visualizer | 归档候选 | rm_vis 循环守护，不用于主链路 |
| `desktop_start_full_stack.sh` | legacy | 归档候选 | 旧版桌面全栈启动 |
| `desktop_start_headless_stack.sh` | legacy | 归档候选 | 旧版桌面无头启动 |

### 顶层 scripts/

| 脚本 | 类别 | 推荐状态 | 说明 |
|---|---|---|---|
| `scripts/build_ros2_mainline.sh` | 构建 | 保留 | ROS2 主线构建 |
| `scripts/build_firmware_mainline.sh` | 构建 | 保留 | 固件构建 |
| `scripts/run_ros2_bridge.sh` | legacy | 归档候选 | 旧版桥接启动 |
| `scripts/tianaim_paths.sh` | 工具 | 保留 | 路径变量定义 |
| `scripts/serve_single_file.ps1` | unknown | 可删除候选 | 临时文件服务脚本 |

## 3. 可删除候选

以下文件列为可删除候选，但 **所有删除项都需要用户确认**，本轮不执行删除。

| 文件 | 删除风险 | 建议处理 | 需要用户确认 |
|---|---|---|---|
| `scripts/serve_single_file.ps1` | 低 | 删除或移到 archive/ | 是 |
| `ros2_ws/scripts/rm-autoaim.service` | 低 | 移到 archive/ | 是 |
| `ros2_ws/scripts/rm-bridge.service` | 低 | 移到 archive/ | 是 |
| `ros2_ws/scripts/install_autostart_on_rdk_x5.sh` | 低 | 移到 archive/ | 是 |
| `ros2_ws/scripts/install_desktop_launchers_on_rdk_x5.sh` | 低 | 移到 archive/ | 是 |
| `ros2_ws/scripts/start_autoaim_tmux.sh` | 中 | 标记 legacy，不删除 | 是 |
| `ros2_ws/scripts/start_rm_bridge_tmux.sh` | 中 | 标记 legacy，不删除 | 是 |
| `ros2_ws/scripts/start_rm_vis_tmux.sh` | 中 | 标记 legacy/visualizer，不删除 | 是 |
| `ros2_ws/scripts/run_rm_vis_loop.sh` | 中 | 标记 legacy/visualizer，不删除 | 是 |
| `ros2_ws/scripts/desktop_start_full_stack.sh` | 中 | 标记 legacy，不删除 | 是 |
| `ros2_ws/scripts/desktop_start_headless_stack.sh` | 中 | 标记 legacy，不删除 | 是 |
| `ros2_ws/scripts/run_ros2_bridge.sh` (顶层) | 低 | 标记 legacy | 是 |
| `datasets/manifests/test_capture_session.json` | 低 | 测试用例，可保留 | 是 |
| `datasets/raw/test_capture_session/README.txt` | 低 | 测试说明，可保留 | 是 |

### 注意

- `rm_armor_detection` 和 `rm_vehicle_detection` 作为历史/备用模块保留，不删除
- `tools/training/` 下的训练工具保留，作为数据处理工具链
- `firmware/` 全部保留，不涉及本轮清理

## 4. 产品化目录建议

未来建议的目录结构（不移动文件，仅规划）：

```text
gimbal_system/
├── README.md                          # 项目主页
├── AGENTS.md                          # AI agent 协作约定
├── ros2_ws/
│   ├── src/
│   │   ├── hik_camera/                # 相机驱动 [core]
│   │   ├── rm_bear_detection/         # bear 检测 [core]
│   │   ├── rm_gimbal_bridge/          # 云台桥接 [core]
│   │   ├── rm_interfaces/             # 消息定义 [core]
│   │   ├── rm_utils/                  # 工具库 [core]
│   │   ├── rm_vehicle_detection/      # [legacy/optional]
│   │   └── rm_armor_detection/        # [legacy/optional]
│   └── scripts/
│       ├── start_fast_follow_verified.sh   # [唯一推荐入口]
│       ├── run_hik_cam_loop.sh             # [底层 loop]
│       ├── run_rm_det_loop.sh              # [底层 loop]
│       ├── run_rm_bridge_loop.sh           # [底层 loop]
│       ├── profile_fast_follow_link.sh     # [diagnostics]
│       ├── check_autoaim_topics.sh         # [diagnostics]
│       ├── deploy_to_rdk_x5.sh             # [deployment]
│       └── README.md
├── firmware/
│   └── stm32_gimbal_control/          # STM32 固件 [core]
├── scripts/                           # 顶层构建/路径工具
├── tools/                             # 数据/训练/评估工具
├── datasets/                          # 数据集骨架
├── models/                            # 模型元数据
├── assets/                            # README 图片
├── archive/                           # 审计/恢复说明
└── docs/
    ├── current_health_report.md
    ├── codebase_cleanup_audit.md
    ├── architecture.md
    ├── backlog.md
    └── ...
```

## 5. 下一步最小收口方案

1. **补 docs**（本轮完成）：生成 health report、cleanup audit，更新 README
2. **标记 legacy**：在 README 和脚本头部标注哪些是 legacy/visualizer/deployment
3. **下一轮再决定删除/移动**：等用户确认后，将 legacy 脚本移到 archive/ 或标记废弃
