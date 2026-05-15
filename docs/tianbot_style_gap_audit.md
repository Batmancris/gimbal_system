# TianAim 与 Tianbot 风格差距审计

> 基于 rosecho、osight_lidar、tianbot_mini 公开仓库和公司内部规范文档

## 1. 目录结构对比

### 1.1 Tianbot 参考结构（ROS1 catkin）
```
package_name/
├── CMakeLists.txt
├── package.xml
├── README.md
├── launch/
│   └── package_name.launch
├── config/
│   └── params.yaml
├── scripts/
│   └── *.py
├── include/package_name/
│   └── *.h/*.hpp
└── src/
    └── *.cpp
```

### 1.2 TianAim 当前结构（ROS2 ament）
```
ros2_ws/src/
├── hik_camera/
│   ├── CMakeLists.txt
│   ├── package.xml
│   ├── README.md
│   ├── config/
│   │   ├── camera_info.yaml
│   │   └── camera_params.yaml
│   └── launch/
│       └── hik_camera.launch.py
├── rm_bear_detection/
│   ├── CMakeLists.txt
│   ├── package.xml
│   ├── README.md
│   ├── config/
│   │   └── rm_bear_detection.yaml
│   └── launch/
│       └── rm_bear_detection.launch.py
├── rm_gimbal_bridge/
│   ├── CMakeLists.txt
│   ├── package.xml
│   ├── README.md
│   ├── config/
│   │   ├── rm_gimbal_bridge.yaml
│   │   └── ...
│   └── launch/
│       ├── rm_gimbal_bridge.launch.py
│       └── rm_autoaim_system.launch.py
├── rm_interfaces/
│   ├── CMakeLists.txt
│   ├── package.xml
│   └── README.md
└── rm_utils/
    └── CMakeLists.txt
```

### 1.3 差距分析
- **一致**：基本遵循 ROS 包结构，有 launch/, config/, README.md
- **差异**：TianAim 使用 ROS2 (ament_cmake)，参考仓库是 ROS1 (catkin)，这是合理的版本差异

## 2. Package.xml 对比

### 2.1 Tianbot 风格
```xml
<package format="2">
  <name>package_name</name>
  <version>0.0.0</version>
  <description>Description</description>
  <maintainer email="email">name</maintainer>
  <license>BSD</license>
  <buildtool_depend>catkin</buildtool_depend>
  <!-- dependencies -->
</package>
```

### 2.2 TianAim 现状
- 使用 ament_cmake（ROS2 标准）
- 依赖声明较完整

### 2.3 差距
- **可改进**：license 字段建议统一为 BSD-3-Clause
- **可改进**：maintainer 信息应保持一致

## 3. CMakeLists.txt 对比

### 3.1 Tianbot 风格
- 标准 catkin 结构
- install 规则完整（TARGETS, DIRECTORY）
- 清晰的 build/install/test 分区

### 3.2 TianAim 现状
- 使用 ament_cmake
- 需要检查 install 规则是否完整

### 3.3 必须检查
- [ ] 所有节点都有 install 规则
- [ ] launch 文件被安装
- [ ] config 文件被安装

## 4. README 结构对比

### 4.1 Tianbot 风格（rosecho 示例）
```markdown
# Package Name
简短描述

## Installation Instructions
安装步骤

## Usage Instructions
使用方法

## Topics
### Published
- /topic_name

## Services
- /service_name

## License
BSD 3-Clause License
```

### 4.2 TianAim 现状
- 有多个 README（根目录、ros2_ws/、各子包）
- 内容较详细，但结构不统一

### 4.3 必须改进
- [ ] 统一 README 结构
- [ ] 移除冗余信息
- [ ] 添加标准 Topics/Services 章节
- [ ] 明确 License

## 5. 命名规范对比

### 5.1 Tianbot 规范
- 包名：小写下划线（rosecho, osight_lidar, tianbot_mini）
- 文件名：描述性，英文
- Topic/Service：遵循 ROS 命名约定

### 5.2 TianAim 现状
- 包名：rm_bear_detection, rm_gimbal_bridge（符合规范）
- Topic：/hbmem_img, /bear_detection/targets（符合规范）

### 5.3 差距
- **基本符合**，命名规范执行良好

## 6. 脚本管理对比

### 6.1 Tianbot 风格
- scripts/ 目录存放 Python 脚本
- launch 文件使用 .launch 或 .launch.py

### 6.2 TianAim 现状
- ros2_ws/scripts/ 存放大量 .sh 脚本
- 有重复脚本（多个启动脚本功能重叠）
- 有 legacy 脚本（tmux 相关、rm_vis 相关）

### 6.3 必须改进
- [ ] 清理重复脚本
- [ ] 标记 legacy 脚本
- [ ] 统一启动入口

## 7. 文档管理对比

### 7.1 Tianbot 风格
- 维护看云产品使用手册
- README 为主要技术文档

### 7.2 TianAim 现状
- docs/ 目录有大量过程文档
- 有多个 runbook、health report、audit 文件
- 文档职责不清

### 7.3 必须改进
- [ ] 归档过程文档到 docs/archive/
- [ ] 保留唯一权威 runbook
- [ ] 简化 docs/ 结构

## 8. Git 工作流对比

### 8.1 Tianbot 规范
- Conventional commits
- 不用 git add .
- Feature 分支工作流
- 每天至少一个 commit

### 8.2 TianAim 现状
- Commit message 基本遵循 conventional commits
- 分支：feature/formula-mini-kt-cloud-follow

### 8.3 差距
- **基本符合**，工作流执行良好

## 9. 对齐优先级

### 必须改（第一轮）
1. README 结构统一
2. docs/ 归档过程文档
3. scripts/ 标记 legacy
4. 确认 CMakeLists.txt install 规则完整

### 可选改（第二轮）
1. 补充 package.xml license 信息
2. 统一 maintainer 信息
3. 清理重复脚本

### 不建议改
1. ROS2 vs ROS1 架构差异（合理的技术选型）
2. 已有的命名规范（已符合）

### 不能直接照搬
1. catkin 构建系统（应保持 ament_cmake）
2. ROS1 launch 文件格式（应保持 ROS2 launch.py）
3. 特定硬件相关的配置

## 10. 验收标准

完成对齐后应满足：
- [ ] 每个 ROS 包都有标准 README 结构
- [ ] docs/ 只保留当前有效文档
- [ ] scripts/ 有清晰的 README 说明
- [ ] legacy 脚本已标记或归档
- [ ] CMakeLists.txt install 规则完整
- [ ] 无冗余重复文件

---

*审计时间：2026-05-15*
*参考仓库：rosecho, osight_lidar, tianbot_mini*
*参考文档：公司内部开发者规范（已提炼）*
