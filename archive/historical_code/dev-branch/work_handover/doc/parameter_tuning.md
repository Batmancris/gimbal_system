# 可调式参数表  

## 1. 相机节点相关  

_.\rm_camera_driver\src\daheng_camera.cpp_  

|参数|描述|估计范围|默认值|
|------|-------|-------|-------|
|`frame_rate_`|帧率| 0~210 | 210|
|`exposure_time`|曝光时间| 0~10000 | 2000|
|`offsetX、Y`|xy偏移量| 0~350 | 0 |
|`rmw_qos_profile_sensor_data`|QoS| reliable/Best-Effort| reliable|
|`auto_white_balance_`|白平衡开关| 0/1 |1|


## 2. 传统识别器相关  

_.\armor_detector\src\armor_detector.cpp_  

| 参数 | 描述 | 估计范围 | 默认值 |
|------|-------|----------|--------|
| `light.min_ratio` | 灯条最小宽高比（短边/长边） | 0.05-0.15 | 0.08 |
| `light.max_ratio` | 灯条最大宽高比 | 0.3-0.6 | 0.4 |
| `light.max_angle` | 灯条最大倾斜角度（度） | 30-60 | 40.0 |
| `light.color_diff_thresh` | 红蓝颜色差异阈值 | 10-50 | 25 |
| `armor.min_light_ratio` | 两个灯条长度比最小值（短/长） | 0.4-0.8 | 0.6 |
| `armor.min_small_center_distance` | 小装甲板灯条中心最小距离（单位：灯条长度） | 0.5-1.5 | 0.8 |
| `armor.max_small_center_distance` | 小装甲板灯条中心最大距离 | 2.5-4.0 | 3.2 |
| `armor.min_large_center_distance` | 大装甲板灯条中心最小距离 | 3.0-4.0 | 3.2 |
| `armor.max_large_center_distance` | 大装甲板灯条中心最大距离 | 4.0-7.0 | 5.0 |
| `armor.max_angle` | 两个灯条中心连线最大角度（度） | 20-45 | 35.0 |
| `binary_thres` | 二值化阈值 | 0-255 | 160 |

## 3. 传统识别器节点相关  

_.\armor_detector\src\armor_detector_node.cpp_  

| 参数 | 描述 | 范围/可选值 | 默认 |
|------|-------|-------------|------|
| `target_frame` | 目标坐标系 | odom, map, base_link等 | odom |
| `debug` | 调试模式开关 | true/false | true |
| `classifier_threshold` | 数字分类置信度阈值 | 0.5-0.95 | 0.7 |
| `ignore_classes` | 忽略的数字类别 | ["negative", "other"] | ["negative"] |
| `use_ba` | 使用Bundle Adjustment优化 | true/false | true |
| `use_pca` | 使用PCA校正灯条角点 | true/false | true |
| `armor_detector.result_img.jpeg_quality` | 结果图像JPEG质量 | 1-100 | 50 |
| `armor_detector.binary_img.jpeg_quality` | 二值图JPEG质量 | 1-100 | 50 |

## 4. yolo后处理相关  

_.\rm_armor_detection\src\parser.cpp_  

| 参数 | 描述 | 估计范围 | 默认值 |
|------|-------|----------|--------|
| `score_threshold_` | 检测分数阈值 | 0.1-0.9 | 0.4 |
| `conf_thres_raw` | Sigmoid反函数计算原始阈值 | 自动计算 | -log(1/0.4-1) |
| `nms_threshold_` | 非极大值抑制阈值 | 0.1-0.9 | 0.5 |
| `nms_top_k_` | NMS前保留的最大候选框数 | 1000-10000 | 5000 |
| `kpt_num_` | 关键点数量（装甲板角点） | 4 |/|
| `kpt_encode_` | 每个关键点编码维度 | 3 (x,y,score) |/|
| `class_num_` | 类别数量 | 18 |/|
| `reg_` | 边界框回归参数维度 | 16 |/|
| `strides` | 特征图下采样步长 | [8, 16, 32] |/|
| `anchors_table` | 锚点配置 | 三组预设值 |/|
| `class_names` | 类别名称映射 | 蓝/红方装甲板类别 |/|

## 5. yolo推理节点参数

_.\rm_armor_detection\src\sample.cpp_  

| 参数 | 描述 | 值/可选值 |
|------|-------|----------|
| `model_file` | 模型文件路径 | "./config/yolo8_pose_bayese_640x640_nv12_modified.bin" |
| `model_task_type` | 推理任务类型 | ModelInferType |
| `task_num` | 并行推理任务数 | 1-8 (默认4) |
| `model_input_width_` | 模型输入宽度 | 640 |
| `model_input_height_` | 模型输入高度 | 640 |
| `x_ratio`, `y_ratio` | 图像缩放比例 | 动态计算 |
| 订阅话题 | 图像输入 | "/hbmem_img" |
| 发布话题 | 检测结果 | "/dnn_node_sample" |
| QoS策略 | 图像订阅 | SensorDataQoS |
| 图像缩放限制 | 宽度16倍数，高度偶数 | 固定要求 |

## 6. 上位机串口相关  

_.\rm_armor_detection\src\sample.cpp_  

| 参数 | 描述 | 值/范围 |
|------|-------|--------|
| `serial_port` | 串口设备路径 | "/dev/ttyS1" |
| `baudrate` | 波特率 | 921600 |
| `tx_buf` | 发送数据格式 | "x_predict: %.2f   y_predict:%.2f\n" |
| 固定偏移量 | 中心点坐标修正值 | x:325, y:250 |

## 7. 下位机串口相关  

_.\Gimbal control\Src\coordinate.c_  

### 7.1 接收协议

| 参数 | 描述 | 当前值 | 可调范围/说明 |
|------|-------|--------|---------------|
| 数据包长度 | 接收数据的字节数 | 8字节 | 可调整（需要同步修改协议） |
| 帧头字节1 | 数据包起始标识1 | 0xFA | 可自定义（0x00-0xFF） |
| 帧头字节2 | 数据包起始标识2 | 0xFB | 可自定义（0x00-0xFF） |
| 帧尾字节1 | 数据包结束标识1 | 0xFC | 可自定义（0x00-0xFF） |
| 帧尾字节2 | 数据包结束标识2 | 0xFD | 可自定义（0x00-0xFF） |
| `RX_BUFFER_SIZE` | DMA接收缓冲区大小 | 16 | 数据包长度的2-4倍 |

注：更改后需要同步修改上位机数据包结构

### 7.2 数据解析参数

| 参数 | 描述 | 当前配置 | 可调整项 |
|------|-------|----------|----------|
| 字节序 | 数据字节顺序 | 低字节在前 | 可改为高字节在前 |
| X坐标位置 | X坐标数据在数据包中的位置 | data[2]-data[3] | 可调整位置索引 |
| Y坐标位置 | Y坐标数据在数据包中的位置 | data[4]-data[5] | 可调整位置索引 |
| 数据类型 | 坐标数据类型 | uint16_t | 可改为int16_t或float等 |
| 坐标解析方式 | X/Y坐标的计算方式 | `(data[3] << 8) | data[2]` |

### 7.3 状态控制参数

| 参数 | 描述 | 触发条件 | 可调整项 |
|------|-------|----------|----------|
| `data_ready`标志 | 数据就绪标志 | 接收到完整数据包时置1 | 可改为其他状态机制 |
| 数据校验 | 数据包校验方式 | 仅帧头帧尾校验 | 可增加CRC校验 |

## 8. 云台外层串级PD相关 [**重点**]

_E:\Project\C++\Gimbal control\Src\gimbal_task.c_ _line832_  

|data|description|range|
|------|-------|-------|
|`kp_yaw`|yaw比例系数| 0~100.0 |
|`kp_pitch`|pitch比例系数| 0~100.0|
|`kd_yaw`|yaw微分系数| 0~100.0 |
|`kd_pitch`|pitch微分系数| 0~100.0 |
|`ki_yaw`|yaw积分系数| / |
|`ki_pitch`|pitch积分系数| / |

TODO：  

1. 增加视觉坐标积分环节或设立死区，消除静止时跟踪抖动。

2. 调优kp、kd，提高跟踪性能。
