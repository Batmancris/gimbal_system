# firmware

这里保存下位机固件。当前正式主线是：

```text
firmware/stm32_gimbal_control/
```

## 当前状态

- STM32 固件已经接入 `rm_gimbal_bridge -> USB CDC -> vision_input -> gimbal_task`。
- 低速跟随已经现场验证为顺滑。
- 高速跟随仍然会跟不上，不能写成完成状态。
- 继续保留 UART/旧视觉帧解析兼容路径，直到确认完全不再需要。

## 重点阅读

- `stm32_gimbal_control/README.md`
- `stm32_gimbal_control/Src/vision_input.c`
- `stm32_gimbal_control/Src/target_state.c`
- `stm32_gimbal_control/Src/gimbal_task.c`
- `stm32_gimbal_control/Src/gimbal_task.h`

