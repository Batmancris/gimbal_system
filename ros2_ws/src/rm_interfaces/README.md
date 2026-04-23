# rm_interfaces

`rm_interfaces` 保存项目自定义 ROS2 接口。当前主线检测输出主要使用 `ai_msgs/msg/PerceptionTargets`，桥接节点再把目标中心转成 STM32 视觉帧。

当前状态：

- bear 低速跟随已经顺滑。
- 高速跟随仍然存在跟不上。
- 接口层当前不是高速问题的主要瓶颈，除非后续需要新增更丰富的目标速度、时间戳或诊断字段。

