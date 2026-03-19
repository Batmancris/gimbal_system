# rm_camera_driver_nv12

FYT视觉24赛季工业相机驱动模块

## fyt::DahengCameraNode

大恒相机驱动节点

### 发布话题 

*  `hbmem_img` (`hbm_img_msgs/msg/HbmMsg1080P`) - 相机采集到的图像
*  `camera_info` (`sensor_msgs/msg/CameraInfo`) - 相机内参
  
### 参数 

* `camera_info_url` (string, default: "package://rm_bringup/config/camera_info.yaml") - camera_info.yaml文件的路径
* `exposure_time` (int, default: 2000) - 相机曝光时间
* `gain` (double, default: 15.0) - 相机增益
* `resolution_width` (int, default: 1280) - 图像宽
* `resolution_height` (int, default: 1024) - 图像高
* `recording` (bool, default: false) - 是否录制视频 

### 启动指令
```
ros2 run rm_camera_driver_nv12 rm_camera_driver_nv12_node
```
