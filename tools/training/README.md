# tools/training

这里放训练、导出和量化相关工具。

当前运行主线是 bear 检测，低速跟随已经顺滑；高速跟随仍然跟不上时，不要默认认为一定是模型问题，需要结合桥接和下位机诊断一起判断。

训练侧后续重点：

- 补高速运动和运动模糊样本。
- 对比不同阈值下的丢检率和误检率。
- 导出模型后确认 RDK X5 上的 tensor 格式、box format 和输入尺寸。
- 同步更新 `ros2_ws/src/rm_bear_detection/README.md`。

