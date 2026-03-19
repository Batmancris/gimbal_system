# rm_camera_driver_nv12

## fyt::DahengCameraNode

DaHeng Camera Driver Node

### Publish topic

*  `hbmem_img` (`hbm_img_msgs/msg/HbmMsg1080P`) - Images captured by the camera
*  `camera_info` (`sensor_msgs/msg/CameraInfo`) - Camera internal reference
  
### parameter 

* `camera_info_url` (string, default: "package://rm_bringup/config/camera_info.yaml") - camera_info.yaml file path
* `exposure_time` (int, default: 2000) - Camera exposure time
* `gain` (double, default: 15.0) - Camera gain
* `resolution_width` (int, default: 1280) - Image width
* `resolution_height` (int, default: 1024) - Image high
* `recording` (bool, default: false) - If record video

### Run command
```
ros2 run rm_camera_driver_nv12 rm_camera_driver_nv12_node
```
