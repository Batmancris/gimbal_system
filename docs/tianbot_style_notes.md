# Tianbot 工程规范提炼

> 来源：公司内部开发者文档（已提炼为规范要点，不包含敏感信息）

## 1. C++/ROS 命名规范

### 1.1 代码风格基础
- **C++ 规范**：遵循 ROS 官方 CppStyleGuide
- **自动格式化**：使用 PickNikRobotics/roscpp_code_format
- **参考仓库**：rosecho, osight_lidar（C++ 写得比较规范，ROSECHO 部分命名不太注意）

### 1.2 命名要求
- **描述性命名**：提倡 descriptive/informative 的文件名、函数名
- **英文基础**：需要良好的英文命名能力
- **包命名**：遵循 REP 144 (ROS Package Naming)
- **坐标系**：遵循 REP 105 (Coordinate Frames for Mobile Platforms)
- **IMU**：遵循 REP 145 (Conventions for IMU Sensor Drivers)
- **Laser**：遵循 REP 138 (LaserScan Common Topics, Parameters, and Diagnostic Keys)

### 1.3 ROS 开发规范
- 参考 ROS Wiki DevelopersGuide
- 参考 wsnewman/ros_class 学习类的写法

## 2. Python/脚本规范

### 2.1 Python 风格
- **基础规范**：遵循 PEP 8
- **风格理念**：PEP 8 不是严格规范，优先可读性，智能判断
- **参考**：ROS Wiki PyStyleGuide

### 2.2 Shell 脚本
- 使用 bash 或 zsh
- tmux 作为终端复用工具

## 3. Git Commit 和 Branch 工作流

### 3.1 Commit Message 规范
采用 Conventional Commits 格式：

| 类型 | 说明 |
|------|------|
| feat | 新增或移除功能 |
| fix | 修复 bug |
| refactor | 重构代码，不改变 API 行为 |
| perf | 性能优化的特殊重构 |
| style | 不影响含义的格式调整 |
| test | 添加或修正测试 |
| docs | 仅影响文档 |
| build | 影响构建组件（构建工具、CI、依赖、版本） |
| ops | 影响运维组件（基础设施、部署、备份、恢复） |
| chore | 杂项修改（如 .gitignore） |

### 3.2 Commit 粒度
- 每个 commit 任务粒度要适中
- 每次 review 至少应该有一个 commit
- 子任务粒度不超过两天，应该是一次 commit 的量
- **每天至少产生一个 commit**，保证提交的代码可运行

### 3.3 分支工作流（混合工作流）
```
main/master <- dev <- feature
```

- **main 分支**：始终保持可部署状态
- **upstream**：tianbot 官方仓库
- **origin**：开发者 fork 的仓库
- **feature 分支**：从 dev 创建，完成后向 dev 合并，然后删除

### 3.4 Git 操作规范
- **暂存文件**：用 `git add <指定文件>`，**不要用 `git add .`**
- **合并方向**：全部向上合并，不要向下合并
- **PR 流程**：feature -> dev (fork)，dev -> dev (upstream)
- **Sync Fork**：通过 GitHub 的 Sync Fork 按钮同步
- **Merge 策略**：
  - Allow merge commits：主要使用
  - Allow squash and merge：feature 向 dev 合并时可选
  - Allow rebase and merge：谨慎使用，会重写 commits

## 4. README/文档规范

### 4.1 文档体系
- **产品使用手册**：主要维护看云文档
- **代码仓库文档**：README 为主
- **readthedocs**：历史遗留，未认真维护

### 4.2 文档要求
- 开发文档应能在 ROS2GO 环境中复现
- 文档撰写规范见 5_文档撰写（未提供）

## 5. 任务粒度和 Code Review 要求

### 5.1 任务管理
- **项目管理工具**：ONES 平台
- **工作项类型**：需求、任务
- **任务状态**：未开始、进行中、挂起、测试、已完成、已关闭
- **子任务状态**：未开始、进行中、已完成

### 5.2 任务粒度
- 子任务粒度不超过两天
- 子任务应该保证能完成
- 软件开发子任务 = 一次 commit

### 5.3 Code Review
- PR 是最重要的协作方式
- 合并前必须代码审查
- 审查者需熟悉 Tianbot Coding Style
- 保持 main 分支始终可部署，降低合并冲突

## 6. 对 TianAim 项目的直接约束清单

### 6.1 命名约束
- [ ] C++ 类名、函数名、成员变量需符合 ROS 命名规范
- [ ] ROS 包名遵循 REP 144
- [ ] Topic/Parameter 命名遵循 ROS 约定

### 6.2 代码风格约束
- [ ] C++ 代码使用 roscpp_code_format 自动格式化
- [ ] Python 代码遵循 PEP 8
- [ ] 描述性文件名和函数名

### 6.3 Git 工作流约束
- [ ] Commit message 使用 conventional commits 格式
- [ ] 不使用 `git add .`，指定具体文件
- [ ] 每天至少一个可运行的 commit
- [ ] Feature 分支从 dev 创建，完成后合并回 dev 并删除
- [ ] 保持 main 分支始终可部署

### 6.4 文档约束
- [ ] README 简洁清晰
- [ ] 关键操作步骤可复现
- [ ] 不在公开仓库暴露内部信息

### 6.5 任务管理约束
- [ ] 子任务粒度 ≤ 2 天
- [ ] 每个子任务对应一次 commit
- [ ] 及时更新任务状态

---

*注：本文档已提炼自公司内部文档，不包含敏感信息（内网地址、账号、密码等）*
*原始文档存放于 private_refs/tianbot_guidelines/，不会提交到公开仓库*
