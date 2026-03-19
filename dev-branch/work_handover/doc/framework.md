# RM Armor Tracker 软件架构说明

## 一、总体架构概述

本软件基于 **ROS2 Humble** 框架开发，采用**上位机-下位机**分离架构，两者通过**CAN总线**进行通信，实现图像处理与电机控制的解耦设计。
<pre>
┌─────────────────────────────────────────────┐
│ 上位机（ROS2） │
├─────────────────────────────────────────────┤
│ 图像采集 → 图像处理 → 目标跟踪 → 控制指令生成 │
└─────────────────────────────────────────────┘
                            │
                    [CAN总线通信]
                            │
┌─────────────────────────────────────────────┐
│ 下位机（FreeRTOS） │
├─────────────────────────────────────────────┤
│ 指令解析 → PID控制 → 电机驱动 → 状态反馈 │
└─────────────────────────────────────────────┘
</pre>

---

## 二、上位机软件架构

### 2.1 传统视觉方案

参考中南大学方案，基于 `rm_vision` 开源框架进行改写，包含两个核心ROS2节点：  

|node|function|
|------|-------|
|`rm_camera_driver`|采集器|  
|`armor_detector`|检测器|  

### I. 图像采集节点 (`rm_camera_driver`)

---
**_功能说明_**：  
负责相机初始化、参数配置和图像话题发布。

**_核心功能实现_**：

**QoS传输策略设置**  

BOF设置决定图像数据的传输可靠性。在RoboMaster竞赛中，为了平衡延迟和可靠性，通常采用以下策略：

- **可靠传输（Reliable）**：保证消息的送达，但可能会重传，延迟可能较高
- **最佳努力传输（Best-Effort）**：不保证送达，但尽力传输，延迟较低

    _./src/rm_armor_driver/src/daheng_camera.cpp_

    ```cpp
    // 设置相机传输策略
    bool use_sensor_data_qos = this->declare_parameter("use_sensor_data_qos", true);
    auto qos = use_sensor_data_qos ? rmw_qos_profile_sensor_data : rmw_qos_profile_default;
    pub_ = image_transport::create_camera_publisher(this, "image_raw", qos);
    ```

    `rmw_qos_profile_sensor_data` 是ROS2中为传感器数据设计的一种QoS配置，它通常采用最佳努力（best-effort）的可靠性策略，因为传感器数据通常是连续且实时性要求高，允许丢失部分数据。而默认的QoS配置通常是可靠（reliable）的，适用于不允许丢失数据的场景。

**帧率、曝光等参数设置**  

传统算法需要对图像进行去噪、二值化、形态学滤波、特征匹配等一系列处理，因此需要尽可能提取显著特征，或将特征与背景作以区分，低曝光、高帧率成为主流选择，低曝光场景下收到光流变化影响较小，同时能避免被反射光或环境纹理影响误识别，需要在初始化时对相机参数做简单调整。

- **帧率（fps）**：每秒传输图像数量

- **曝光时间（exposure_time）**：每个周期内曝光时间

    _./src/rm_armor_driver/src/daheng_camera.cpp_

    ```cpp
    // 设置相机传输策略
    frame_id_ = this->declare_parameter("camera_frame_id", "camera_optical_frame");
    pixel_format_ = this->declare_parameter("pixel_format", "rgb8");
    resolution_width_ = this->declare_parameter("resolution_width", 1280);
    resolution_height_ = this->declare_parameter("resolution_height", 1024);
    auto_white_balance_ = this->declare_parameter("auto_white_balance", 1);
    frame_rate_ = this->declare_parameter("frame_rate", 210);
    exposure_time_ = this->declare_parameter("exposure_time", 2000);
    gain_ = this->declare_parameter("gain", 5.0);
    offest_x_ = this->declare_parameter("offsetX", 0);
    offset_y_ = this->declare_parameter("offsetY", 0);
    ```

### II. 图像推理节点 (`armor_detector`)

[包含较多待补充内容]  
[tf变换]云台坐标系与地盘坐标系，静态广播使用，坐标系依赖关系  
[识别器]灯条检测、大小灯条匹配  
[识别器节点]识别器的实例化、图像消息处理  
[BA光流]弱光环境补偿  
[数字识别]板块分割、全连接网络  

### 2.2 深度学习方案

参考wunuo社区方案，包含两个核心ROS2节点：  

|node|function|
|------|-------|
|`rm_camera_driver_nv12`|采集器|  
|`rm_armor_detector`|yolov8检测器|  

#### 2.2.1 图像采集节点 (rm_camera_driver_nv12)  

- 在传统图像采集基础上，增加NV12格式转换功能，以满足YOLOv8推理节点的输入要求。

    NV12是一种YUV颜色编码格式，广泛用于视频编码和硬件加速。YOLOv8在RDK X5平台上使用BPU进行推理，需要NV12格式输入以提高处理效率。

    _.\rm_camera_driver_nv12\src\daheng_camera.cpp_

    ```cpp
    inline void RGB24_to_NV12(const unsigned char* pRGB, unsigned char* pNV12, int width, int height)
    {
        const uint8x8_t u8_zero = vdup_n_u8(0);
    ...

        unsigned char* UVPtr = pNV12 + width * height;
        int pitch = width >> 4;

        for (int j = 0; j < height; ++j) {
            for (int i = 0; i < pitch; ++i) {
                // Load RGB 16 pixel
                uint8x16x3_t pixel_rgb = vld3q_u8(pRGB);
    ...

                    uint8x8x2_t result;
                    result.val[0] = vqmovn_u16(unsigned_u);
                    result.val[1] = vqmovn_u16(unsigned_v);

                    vst2_u8(UVPtr, result);
                    UVPtr += 16;
                }

                pRGB += 3 * 16;
                pNV12 += 16;
            }
        
    }
    ```

#### 2.2.2 图像推理节点 (rm_armor_detection)

- 基于RDK社区示例程序改写，实现YOLOv8模型推理与串口通信功能。（双线程）

    **核心功能实现：**

    |thread|description|
    |------|-------|
    |`rm_camera_driver_nv12`|推理回调|  
    |`rm_armor_detector`|串口通讯|  

    采用多线程解耦，在一个程序中实现接收图像、运行深度学习模型推理，并将结果通过串口发送多项功能。

- **推理回调线程**

- **串口通信线程**

    为避免推理线程被通信阻塞，在回调函数中开辟第二个线程处理串口通信，确保通信实时性。

    [代码示例]：

    ```cpp
    // 推理结果回调函数
    void InferenceCallback(const std::vector<DetectionResult>& results) {
        // 主线程：处理推理结果
        ProcessDetectionResults(results);
        
        // 创建通信线程
        std::thread comm_thread([results]() {
            // 提取目标坐标
            float x_predict = CalculateTargetX(results);
            float y_predict = CalculateTargetY(results);
            
            // 通过串口发送坐标数据
            SendToSerialPort(x_predict, y_predict);
        });
        
        // 分离线程（异步执行）
        comm_thread.detach();
    }
    ```

## 三、下位机软件架构  

### 3.1 FreeRTOS任务框架

下位机基于FreeRTOS实时操作系统，主要实现电机控制功能。

- 3.1.1 坐标接收任务 (coodinate.cpp)  
负责接收上位机发送的视觉坐标数据，解析自定义通信协议。

    ```c
    // 串口接收中断处理
    void USART1_IRQHandler(void) {
        if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
            uint8_t received_byte = USART_ReceiveData(USART1);
            
            // 状态机解析协议
            static uint8_t state = 0;
            static uint8_t data_index = 0;
            static uint8_t rx_buffer[8];
            
            switch (state) {
                case 0: // 等待消息头第一个字节
                    if (received_byte == 0xFA) {
                        rx_buffer[0] = received_byte;
                        state = 1;
                    }
                    break;
                    
                case 1: // 等待消息头第二个字节
                    if (received_byte == 0xFB) {
                        rx_buffer[1] = received_byte;
                        state = 2;
                        data_index = 2;
                    } else {
                        state = 0; // 重新同步
                    }
                    break;
                    
                case 2: // 接收数据字节
                    rx_buffer[data_index++] = received_byte;
                    if (data_index >= 8) {
                        // 检查消息尾
                        if (rx_buffer[6] == 0xFC && rx_buffer[7] == 0xFD) {
                            // 解析坐标数据
                            int16_t x_raw = (rx_buffer[3] << 8) | rx_buffer[2];
                            int16_t y_raw = (rx_buffer[5] << 8) | rx_buffer[4];
                            
                            // 转换为浮点数
                            target_x = (float)x_raw / 100.0f;
                            target_y = (float)y_raw / 100.0f;
                            
                            // 设置新数据标志
                            new_data_flag = 1;
                        }
                        state = 0; // 重置状态机
                    }
                    break;
            }
            
            USART_ClearITPendingBit(USART1, USART_IT_RXNE);
        }
    }
    ```

- 3.1.2 云台控制任务 (gimbal_task.c)  
功能说明：
实现云台控制主循环，包含初始化、模式设置、数据反馈和控制计算。

    ```c
    // 云台任务主函数
    void gimbal_task(void const *argument) {
        // 云台控制结构体初始化
        gimbal_init(&gimbal_control);
        
        // 设置初始控制模式
        gimbal_set_mode(&gimbal_control);
        
        // 控制模式切换与数据过渡
        gimbal_mode_change_control_transit(&gimbal_control);
        
        // 主控制循环
        while (1) {
            // 1. 云台数据反馈更新
            gimbal_feedback_update(&gimbal_control);
            
            // 2. 设置云台控制量（视觉跟踪）
            gimbal_set_control(&gimbal_control);
            
            // 3. 云台控制PID计算
            gimbal_control_loop(&gimbal_control);
            
            // 4. 任务延时（控制周期）
            osDelay(GIMBAL_CONTROL_PERIOD);
        }
    }

    // 云台视觉跟踪控制
    void gimbal_set_control(gimbal_control_t *gimbal_control) {
        // 检查是否有新视觉数据
        if (new_data_flag) {
            // 计算图像中心偏移量（像素）
            float center_offset_x = target_x - IMAGE_CENTER_X;
            float center_offset_y = target_y - IMAGE_CENTER_Y;
            
            // 计算角度偏差（像素转弧度）
            float angle_offset_x = center_offset_x * PIXEL_TO_RAD_X;
            float angle_offset_y = center_offset_y * PIXEL_TO_RAD_Y;
            
            // 计算角速度（差分计算）
            static float last_offset_x = 0, last_offset_y = 0;
            float delta_x = (angle_offset_x - last_offset_x) / GIMBAL_CONTROL_PERIOD;
            float delta_y = (angle_offset_y - last_offset_y) / GIMBAL_CONTROL_PERIOD;
            
            // 更新历史值
            last_offset_x = angle_offset_x;
            last_offset_y = angle_offset_y;
            
            // 设置云台目标角度和角速度
            gimbal_control->yaw_target_angle = angle_offset_x;
            gimbal_control->pitch_target_angle = angle_offset_y;
            gimbal_control->yaw_target_speed = delta_x;
            gimbal_control->pitch_target_speed = delta_y;
            
            // 清除数据标志
            new_data_flag = 0;
        }
    }
    ```

### 3.2 PID控制

控制架构说明：
下位机采用串级PID控制结构，在底层电机三环PID基础上 ，外层设计两级视觉PD控制。

- **内层PID**

    <pre>
    视觉控制层（上位机）
        │
        ▼
    图像偏差 → PD控制 → 角度指令
        │
        ▼
    位置环PID → 位置指令
        │
        ▼
    速度环PID → 速度指令
        │
        ▼
    电流环PID → PWM输出
        │
        ▼
    电机执行
    </pre>

- **视觉PD：**  

    视觉PD控制器根据图像中的目标位置偏差（center_offset_x）和位置变化率（delta_x）计算云台的角度和角速度指令，实现快速、稳定的目标跟踪。

    ```c
    static void gimbal_set_control(gimbal_control_t *set_control)
    {
        static fp32 add_yaw_angle = 0.0f;
        static fp32 add_pitch_angle = 0.0f;
        const TargetPosition *vision_data = get_target_position();
        static int16_t last_offset_x = 0;
        static int16_t last_offset_y = 0;
        if (set_control == NULL) 
            return;

        // 视觉数据有效处理
        if (vision_data != NULL && vision_data->data_ready) 
        {
            // 计算中心偏移量 (图像中心320x240)
            int16_t center_offset_x = vision_data->object_x - 320;
            int16_t center_offset_y = vision_data->object_y - 240;
            
            // 计算误差变化率 (微分项)
            int16_t delta_x = center_offset_x - last_offset_x;
            int16_t delta_y = center_offset_y - last_offset_y;
            
            // 更新历史值
            last_offset_x = center_offset_x;
            last_offset_y = center_offset_y;

            // 调整比例系数 (响应强度)
            const fp32 PIXEL_TO_RAD = 0.000001f;
            const fp32 kp_yaw = 40.0f;   // 比例系数
            const fp32 kd_yaw = 60.0f;     // 增加阻尼抑制震荡
            const fp32 kp_pitch = 60.0f;
            const fp32 kd_pitch = 40.0;
            
            // 计算角度增量 (带符号处理)
            add_yaw_angle = PIXEL_TO_RAD * (-center_offset_x * kp_yaw - delta_x * kd_yaw);
            add_pitch_angle = PIXEL_TO_RAD * (-center_offset_y * kp_pitch - delta_y * kd_pitch);
            
            // 单次变化量限制 (防止突变)
            const fp32 max_delta = 0.007f;
            add_yaw_angle = constrain(add_yaw_angle, -max_delta, max_delta);
            add_pitch_angle = constrain(add_pitch_angle, -max_delta, max_delta);
            
            // 标记数据已处理
            set_target_data_ready(0);
        }
        else 
        {
            // 无有效数据时缓慢归零
            add_yaw_angle *= 0.7f;
            add_pitch_angle *= 0.7f;
        }

        // 应用增量到电机设定值
        set_control->gimbal_yaw_motor.relative_angle_set += add_yaw_angle;
        set_control->gimbal_pitch_motor.relative_angle_set += add_pitch_angle;

        // 设定值范围约束
        set_control->gimbal_yaw_motor.relative_angle_set = constrain(
            set_control->gimbal_yaw_motor.relative_angle_set, 
            -0.8f, 0.8f);  // 减小机械限幅范围
        
        set_control->gimbal_pitch_motor.relative_angle_set = constrain(
            set_control->gimbal_pitch_motor.relative_angle_set, 
            -0.5f, 0.8f);  // pitch特殊限幅

        // 调试输出
        static uint32_t last_print = 0;
        if (HAL_GetTick() - last_print > 500) {
            usart_printf("[VISION] Target: (%d, %d)\r\n", 
                        vision_data->object_x, vision_data->object_y);
                // 调试输出中添加角度和角速度信息
            // usart_printf("Pitch: Ang=%.3f Gyro=%.3f Err=%.3f\r\n",
            // set_control->gimbal_pitch_motor.relative_angle,
            // set_control->gimbal_pitch_motor.motor_gyro,
            //   add_pitch_angle);
            last_print = HAL_GetTick();
        }

    }
    ```

## 四、通信接口规范  

### 4.1 CAN总线通信协议  

|字段|长度|说明|
|------|-------|-------|
|消息ID|11位| 标识消息类型（0x201:视觉数据，0x202:控制指令） |
|数据长度|1字节| 有效数据长度（0-8字节） |
|数据域|8字节| 具体数据内容 |
|CRC校验|2字节| 循环冗余校验|

### 4.2 自定义消息格式  

| 参数 | 描述 | 当前值 | 可调范围/说明 |
|------|-------|--------|---------------|
| 数据包长度 | 接收数据的字节数 | 8字节 | 可调整（需要同步修改协议） |
| 帧头字节1 | 数据包起始标识1 | 0xFA | 可自定义（0x00-0xFF） |
| 帧头字节2 | 数据包起始标识2 | 0xFB | 可自定义（0x00-0xFF） |
| 帧尾字节1 | 数据包结束标识1 | 0xFC | 可自定义（0x00-0xFF） |
| 帧尾字节2 | 数据包结束标识2 | 0xFD | 可自定义（0x00-0xFF） |
| `RX_BUFFER_SIZE` | DMA接收缓冲区大小 | 16 | 数据包长度的2-4倍 |

---
文档版本: v1.1  
最后更新: 2025年2月25日  
维护状态: 活跃维护中  
适用平台: Ubuntu 22.04 + ROS2 Humble / FreeRTOS  

本架构文档详细说明了RM Armor Tracker的软件实现细节，供项目交接和后续开发参考。