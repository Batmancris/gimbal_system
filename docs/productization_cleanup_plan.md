# TianAim 产品化清理计划

> 基于 Tianbot 风格审计，第一轮只做文档归档和低风险清理

## 文件分类统计

- **总文件数**：422
- **docs/ 目录**：6 个文档
- **ros2_ws/scripts/ 目录**：24 个脚本
- **README 文件**：26 个

---

## A. 必须保留（核心功能）

### ROS2 核心包
| 路径 | 说明 | 状态 |
|------|------|------|
| ros2_ws/src/hik_camera/ | 海康相机驱动 | 保留 |
| ros2_ws/src/rm_bear_detection/ | 熊检测（当前使用） | 保留 |
| ros2_ws/src/rm_gimbal_bridge/ | 云台通信桥 | 保留 |
| ros2_ws/src/rm_interfaces/ | 自定义消息接口 | 保留 |
| ros2_ws/src/rm_utils/ | 工具库 | 保留 |

### 核心启动脚本
| 路径 | 说明 | 状态 |
|------|------|------|
| ros2_ws/scripts/start_fast_follow_verified.sh | **主启动脚本** | 保留 |
| ros2_ws/scripts/start_headless_follow_light.sh | 轻量无头启动 | 保留 |
| ros2_ws/scripts/run_hik_cam_loop.sh | 相机循环启动 | 保留 |
| ros2_ws/scripts/run_rm_det_loop.sh | 检测循环启动 | 保留 |
| ros2_ws/scripts/run_rm_bridge_loop.sh | 桥循环启动 | 保留 |
| ros2_ws/scripts/profile_fast_follow_link.sh | 性能分析 | 保留 |

### STM32 固件
| 路径 | 说明 | 状态 |
|------|------|------|
| firmware/stm32_gimbal_control/ | 下位机固件 | 保留 |

### 工具和数据集
| 路径 | 说明 | 状态 |
|------|------|------|
| tools/ | 训练、标注、评估工具 | 保留 |
| datasets/ | 数据集结构 | 保留 |
| models/ | 模型文件 | 保留 |

---

## B. 保留但标记 Legacy/Optional

### Legacy 检测包
| 路径 | 说明 | 建议 |
|------|------|------|
| ros2_ws/src/rm_armor_detection/ | 装甲检测（未使用） | 标记 legacy |
| ros2_ws/src/rm_vehicle_detection/ | 车辆检测（未使用） | 标记 legacy |

### Legacy 启动脚本
| 路径 | 说明 | 建议 |
|------|------|------|
| ros2_ws/scripts/start_autoaim_tmux.sh | tmux 自瞄启动 | 标记 legacy |
| ros2_ws/scripts/start_rm_bridge_tmux.sh | tmux 桥启动 | 标记 legacy |
| ros2_ws/scripts/start_rm_vis_tmux.sh | tmux 可视化启动 | 标记 legacy |
| ros2_ws/scripts/run_rm_vis_loop.sh | 可视化循环启动 | 标记 legacy |

### 系统服务和桌面启动
| 路径 | 说明 | 建议 |
|------|------|------|
| ros2_ws/scripts/rm-autoaim.service | systemd 服务 | 标记 optional |
| ros2_ws/scripts/rm-bridge.service | systemd 服务 | 标记 optional |
| ros2_ws/scripts/install_autostart_on_rdk_x5.sh | 自启安装 | 标记 optional |
| ros2_ws/scripts/install_desktop_launchers_on_rdk_x5.sh | 桌面启动安装 | 标记 optional |
| ros2_ws/scripts/desktop_start_full_stack.sh | 桌面全栈启动 | 标记 optional |
| ros2_ws/scripts/desktop_start_headless_stack.sh | 桌面无头启动 | 标记 optional |
| ros2_ws/scripts/desktop_status_full_stack.sh | 桌面状态检查 | 标记 optional |
| ros2_ws/scripts/desktop_stop_full_stack.sh | 桌面停止 | 标记 optional |

---

## C. 可移动到 docs/archive/

### 过程报告和审计文档
| 路径 | 说明 | 建议动作 |
|------|------|----------|
| docs/codebase_cleanup_audit.md | 代码库清理审计 | 移动到 docs/archive/ |
| docs/current_health_report.md | 当前健康报告 | 移动到 docs/archive/ |
| docs/migration_plan.md | 迁移计划 | 移动到 docs/archive/ |
| docs/vehicle_detection_integration.md | 车辆检测集成 | 移动到 docs/archive/ |
| docs/backlog.md | 待办事项 | 移动到 docs/archive/ |
| archive/repo_audit_2026-04-11.md | 仓库审计报告 | 已在 archive/，保留 |

### 历史 README
| 路径 | 说明 | 建议动作 |
|------|------|----------|
| ros2_ws/src/rm_armor_detection/README_cn.md | 中文 README | 移动到包内 archive/ 或删除 |
| ros2_ws/src/rm_vehicle_detection/README.md | 车辆检测 README | 标记 legacy |

---

## D. 可删除候选（需用户确认）

### 临时脚本
| 路径 | 说明 | 被引用 | 删除风险 | 建议 |
|------|------|--------|----------|------|
| scripts/serve_single_file.ps1 | PowerShell 单文件服务 | 无 | 低 | 删除 |
| ros2_ws/scripts/usb_cdc_pitch_control_test.py | USB CDC 测试 | 无 | 低 | 移动到 tools/ 或删除 |

### 重复/冗余脚本
| 路径 | 说明 | 被引用 | 删除风险 | 建议 |
|------|------|--------|----------|------|
| ros2_ws/scripts/build_and_run_on_rdk_x5.sh | 构建运行 | 无直接引用 | 低 | 合并到 deploy 脚本或标记 legacy |
| ros2_ws/scripts/clean_build_and_start_on_rdk.sh | 清理构建启动 | 无直接引用 | 低 | 合并或标记 legacy |
| ros2_ws/scripts/deploy_to_rdk_x5.sh | 部署脚本 | 无直接引用 | 低 | 保留但检查是否冗余 |
| ros2_ws/scripts/check_autoaim_topics.sh | 话题检查 | 无直接引用 | 低 | 移动到 tools/ 或保留 |

---

## E. README 整理计划

### 当前 README 分布（26 个）
- 根目录：1 个（主 README）
- ros2_ws/：1 个
- ros2_ws/scripts/：1 个
- ros2_ws/src/：6 个子包 README
- 其他目录：17 个

### 整理建议
1. **保留并优化**：
   - README.md（根目录）- 主文档
   - ros2_ws/README.md - ROS2 工作空间说明
   - ros2_ws/scripts/README.md - 脚本分类说明
   - 各子包 README.md - 包文档

2. **归档或合并**：
   - datasets/ 下的多个 README - 合并到 datasets/README.md
   - tools/ 下的多个 README - 保留（工具文档）
   - archive/README.md - 保留

3. **标记 legacy**：
   - ros2_ws/src/rm_armor_detection/README.md
   - ros2_ws/src/rm_vehicle_detection/README.md

---

## 执行计划

### 第一轮（当前）- 文档收口
1. [ ] 移动过程文档到 docs/archive/
2. [ ] 更新 ros2_ws/scripts/README.md 添加 legacy 标记
3. [ ] 简化根目录 README.md
4. [ ] 更新 ros2_ws/README.md
5. [ ] 不删除任何文件

### 第二轮（用户确认后）- 脚本清理
1. [ ] 删除确认无用的临时脚本
2. [ ] 合并重复的部署脚本
3. [ ] 标记 legacy 脚本

### 第三轮（用户确认后）- 代码清理
1. [ ] 检查 CMakeLists.txt install 规则
2. [ ] 清理未使用的依赖
3. [ ] 统一 package.xml 格式

---

## 验收标准

完成第一轮后：
- [ ] docs/ 只保留当前有效文档
- [ ] docs/archive/ 包含所有过程报告
- [ ] ros2_ws/scripts/README.md 标注了 legacy 脚本
- [ ] 主 README 简洁清晰
- [ ] 无功能代码变更
- [ ] 无删除操作

---

*计划生成时间：2026-05-15*
*当前分支：chore/tianbot-style-productize-v1*
*下一步：执行文档归档*
