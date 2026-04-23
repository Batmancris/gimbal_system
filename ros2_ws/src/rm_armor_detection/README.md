# rm_armor_detection

`rm_armor_detection` 是 armor 兼容检测包。当前默认跟随主线是 bear，不是 armor。

## 当前说明

- 当前默认链路：`hik_camera -> rm_bear_detection -> rm_gimbal_bridge -> STM32`。
- armor 链路保留用于兼容和后续切换。
- 低速跟随顺滑的结论来自当前 bear 主线。
- 高速跟随仍然存在跟不上。

## 兼容话题

```text
/hbmem_img -> rm_armor_detection -> /dnn_node_sample
```

## 运行

```bash
cd /home/sunrise/rm_ws
source /opt/tros/humble/setup.bash
source install/setup.bash
ros2 run rm_armor_detection rm_armor_detection
```

