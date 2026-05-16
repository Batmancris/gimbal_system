# hik_camera

`hik_camera` 是 RDK X5 运行链路的相机入口，负责发布普通 ROS 图像和 TROS shared-memory 图像。

## 当前链路位置

```text
hik_camera -> /hbmem_img -> rm_bear_detection -> /bear_detection/targets
```

当前低速跟随已经顺滑，高速跟随仍然会跟不上。相机侧需要重点确认高速场景下帧率、曝光和运动模糊是否影响检测。

## 常用检查

```bash
ros2 topic info /image_raw -v
ros2 topic info /hbmem_img -v
tmux -L autoaim capture-pane -pt hik_cam
```

> 注意：`/image_raw` 仅在 `publish_image_raw:=true` 时才会发布。默认链路以 `/hbmem_img` 为准，不需要依赖 `/image_raw`。

## 运行

```bash
cd /home/sunrise/rm_ws
source /opt/tros/humble/setup.bash
source install/setup.bash
ros2 launch hik_camera hik_camera.launch.py
```

