# 机甲大师装甲板跟踪

本项目旨在建议一个 **易用** 的识别器和跟踪器。  
其中功能代码包含很多注释, 且 **松耦合** 架构支持单元测试。  
整体部署方案是循序渐进的，可以作为 **初学者** 的实践项目。  
____

[English](./Readme.md) | 简体中文  

## 1. 简介

- **推荐环境**

项目框架基于陈君 **rm_vision** 开源框架。  
硬件设施基于 **RDK_X5** 与 **大恒工业相机**  
软件系统基于 **Ubuntu2204** 和 **ROS2humble**

- **更新日志**

***2025-1-8：上线基本功能***  
<pre>
.  
└── src  
    └── rm_armor_tracker  
        ├── rm_utils*  
        └── rm_camera_driver*  
</pre>
**rm_utils**: 数学工具库  
**rm_camera_driver**: 发布原始图像话题  

***2025-2-6：增加传统识别器***  
<pre>
.  
└── src  
    └── rm_armor_tracker  
        ├── rm_utils  
        ├── rm_camera_driver  
        ├── rm_interfaces*  
        └── armor_detector*  
</pre>
**rm_interfaces**: 自定义消息与服务  
**armor_detector**:灯条识别  

***2025-2-20：增加 Yolov8 识别器***
<pre>
.
└── src
    └── rm_armor_tracker
        ├── rm_utils
        ├── rm_camera_driver
        ├── rm_interfaces
        ├── armor_detector
        ├── rm_camera_driver_nv12*
        └── rm_armor_detection*
  </pre>
**rm_camera_driver_nv12**: 发布 nv12格式 图像话题  
**rm_armor_detection**:Yolov8n 模型推理  

____

## 2. 安装指导

### 2.1 获取源码  

- 创建工作空间

  ```shell
  cd 
  ```  

  ```shell
  mkdir -p rm_ws/src | cd rm_ws
  ```  

- 初始化工作空间  

  ```shell
  colcon build
  ```  

  ```shell
  cd src
  ```  

- 下载源码  

  ```shell
  git clone https://github.com/tianbot/rm_armor_tracker/tree/dev
  ```

____

### 2.2 依赖

更新软件包索引  

  ```shell
  sudo apt-get update
  ```  

依赖列表

- [大恒相机驱动](#221-大恒相机驱动) :硬件驱动  
- [ros2-常用组件](#222-ros2-常用组件) :提供常用功能  
- [数学工具](#223-数学工具) :传统识别器数学工具  
- [rdk_x5-tros-组件](#224-rdk_x5-tros-组件) :Yolov8 算法依赖  

#### 2.2.1 大恒相机驱动

根据你的系统选择安装包版本

- **Aarch64--RDK_X5**

  ```shell
  wget https://gb.daheng-imaging.com/CN/Software/Cameras/Linux/Galaxy_Linux-armhf_Gige-U3_32bits-64bits_1.5.2303.9202.zip
  ```

- **X86_64--Linux**

  ```shell
  wget https://gb.daheng-imaging.com/CN/Software/Cameras/Linux/Galaxy_Linux-x86_Gige-U3_32bits-64bits_1.5.2303.9221.zip
  ```

- **X86_64--Win**

  ```shell
  wget https://gb.daheng-imaging.com/CN/Software/Cameras/Windows/Galaxy_Windows_CN_32bits-64bits_2.4.2501.9211.zip
  ```

解压压缩包

  ```shell
  unzip Galaxy_Linux-armhf_Gige-U3_32bits-64bits_1.5.2303.9202
  ```

运行自动下载脚本

  ```shell
  sudo bash ./Galaxy_camera.run
  ```

#### 2.2.2 ROS2 常用组件

- **camera_info_manager**  

  >其提供了许多相机驱动程序使用的 C++ 类来管理 ROS 图像管道所需的相机校准数据。

  ```shell
  sudo apt-get install ros-humble-camera_info_manager
  ```  

- **image_transport**  

  >image_transport用于订阅和发布图像。  

  ```shell
  sudo apt-get install ros-humble-image_transport
  ```

- **fmt**

  >提供 C stdio 和 C++ iostream 的快速、安全替代方案.

  ```shell
  sudo apt-get install libfmt-dev
  ```

- **spdlog**

  >快速 C++ 日志库

  ```shell
  sudo apt install  libspdlog-dev
  ```

- **absl**

  >Abseil 是 C++ 库代码的开源集合，旨在增强 C++ 标准库。  

  ```shell
   git clone https://github.com/abseil/abseil-cpp.git
  ```

  ```shell
  cd abseil-cpp
  ```
  
  ```shell
   mkdir build && cd build
  ```

  ```shell
  cmake ..
  ```

  ```shell
  make -j
  ```  

  ```shell
  sudo make install  
  ```

- **Qt**

  >Qt 是一个跨平台的桌面、嵌入式和移动应用程序开发框架。

  ```shell
  sudo apt install qtdeclarative5-dev qt5-qmake libqglviewer-dev-qt5
  ```

  如果你想直接尝试AI方案， 跳转到 [2.2.4 RDK_X5 tros 组件](#224-rdk_x5-tros-组件)

#### 2.2.3 数学工具

- **eigen**

  >Eigen 是一个用于线性代数的 C++ 模板库：矩阵、向量、数值求解器和相关算法。

  ```shell
  sudo apt install libeigen3-dev
  ```

- **suitesparse**

  > SuiteSparse是一组稀疏矩阵相关的包

  ```shell
  sudo apt install   libsuitesparse-dev
  ```

- **Ceres**  : <https://github.com/ceres-solver/ceres-solver>

  >Ceres Solver 1 是一个开源 C++ 库，用于建模和解决大型、复杂的优化问题。

  ```shell
  git clone --recurse-submodules https://github.com/ceres-solver/ceres-solver.git
  ```  

  ```shell
  cd ceres-solver-2.2.0
  ```  

  ```shell
  sudo gedit ./CMakeLists.txt
  ```

  find_package(Eigen3 3.3 REQUIRED)  >> find_package(Eigen3  REQUIRED)  
  
  ```shell
  mkdir build && cd build
  ```  

  ```shell
  cmake ..
  ```

  ```shell
  make -j
  ```  

  ```shell
  sudo make install  
  ```  

- **Sophus**  : <https://github.com/strasdat/Sophus>

  >Sophus 是李群的 C++ 实现，常用于 2d 和 3d 几何问题  

  ```shell
  git clone https://github.com/strasdat/Sophus
  ```  

  ```shell
  cd Sophus-main
  ```  

  ```shell
  sudo gedit ./CMakeLists.txt
  ```

  cmake_minimum_required(VERSION 3.24)  >> cmake_minimum_required(VERSION 3.16)

  ```shell
  mkdir build && cd build
  ```  

  ```shell
  cmake ..
  ```

  ```shell
  make -j
  ```  

  ```shell
  sudo make install  
  ```

- **G2O** : <https://github.com/RainerKuemmerle/g2o>

  >g2o 是一个开源 C++ 框架，用于优化基于图的非线性误差函数。  

  ```shell
  git clone https://github.com/RainerKuemmerle/g2o
  ```  

  ```shell
  cd g2o
  ```  

  ```shell
  mkdir build && cd build
  ```  

  ```shell
  cmake ..
  ```

  ```shell
  make -j
  ```  

  ```shell
  sudo make install  
  ```  

#### 2.2.4 RDK_X5 tros 组件

- hobot_msgs : <https://github.com/D-Robotics/hobot_msgs>  

    ```shell
    sudo apt install tros-humble-hbm-img-msgs tros-humble-ai-msgs    
    ```  

    >自定义消息组

- dnn-node : <https://github.com/D-Robotics/hobot_dnn>

    ```shell
    sudo apt install tros-humble-dnn-node   
    ```  

  > 神经网络基类

- hobot-cv : <https://github.com/D-Robotics/hobot_cv>

  ```shell
  sudo apt install tros-humble-hobot-cv   
  ```  

  >BPU 计算加速优化器

- hobot_codec : <https://github.com/D-Robotics/hobot_codec>

  ```shell
  sudo apt install sudo apt install tros-humble-hobot-codec   
  ```  

  >显示ROS2节点的图像消息

- websocket : <https://github.com/D-Robotics/hobot_websocket>

  ```shell
  sudo apt install sudo apt install tros-humble-websocket   
  ```

  >网页端可视化工具  
  
____

## 3. 编译  

### 3.1 传统识别器  

- 编译功能包  

    ```shell
    colcon build  --packages-select rm_utils
    ```  

    ```shell
    colcon build  --packages-select rm_interfaces
    ```  

    ```shell
    colcon build  --packages-select rm_camera_driver
    ```  

    ```shell
    colcon build  --packages-select armor_detector
    ```  

### 3.2 YoLo v8 识别器

- 编译功能包  

    ```shell
    colcon build  --packages-select rm_camera_driver_nv12
    ```  

    ```shell
    colcon build  --packages-select rm_armor_detection
    ```  

### 3.3 将安装空间加入环境变量  

  ```shell
  source install/setup.bash
  ```  

____

## 4. 启动项目

### 4.1 传统识别  

- **启动相机节点**  

  ```shell
  ros2 run rm_camera_driver rm_camera_node
  ```  

  *话题*: /image_raw  

- **TF 静态坐标广播**

  ```shell
  ros2 run tf2_ros static_transform_publisher --frame-id odom --child-frame-id camera_optical_frame --x 0.5 --y 0.0 --z 0.0 --roll 0.0 --pitch 0.0 --yaw 0.0
  ```

- **启用识别器节点**  

  ```shell
  ros2 run armor_detector armor_detector_node
  ```  

  *话题*: /armor_detector/result_img  

- **可视化**  

  - launch rviz2

    ```shell
    rviz2
    ```  

  - Add by topic
  `image_raw`  
  `binary img`  
  `result img`  

  - QoS seting
  `Reliable >> Best Effort`  
  - origin img

    ![origin](./doc/origin.png)

  - binary img

    ![binary](./doc/binary.png)

  - result img

    ![result](./doc/result.png)

  - topic hz

    ![topic](./doc/fps.png)
    Terminal：fps:203(i7 10800H)

### 4.2 YOLOv8

- **启动相机节点**  

  ```shell
  ros2 run rm_camera_driver rm_camera_driver_node
  ```

- **启用Web网页展示**  

  ```shell
  export WEB_SHOW=TRUE
  ```

- **启动识别器**  

  ```shell
  cp -r YOUR_WS/install/rm_armor_detection/lib/rm_armor_detection/config .
  ```

  ```shell
  ros2 launch rm_armor_detection rm_armor_detection.launch.py
  ```

- **Web可视化**  
将"192.168.0.215"改为你当前 RDK 开发板的IP

  ```shell
  firefox https://192.168.0.215:8000
  ```

  - websocket
  ![websocket](./doc/blue.png)
  - tmux
  ![tmux](./doc/45hz.png)
  Terminal：fps:72(RDK_X5)

____

## 5. 话题  

### 5.1 传统识别器

| topic                         | Description  |
| -----------                   | -----------  |
| /image_raw                    | 原始图像      |
| /armor_detector/binary_img    | 二值图        |
| /armor_detector/result_img    | 输出结果      |
| /armor_detector/number_img    | 装甲板编号    |
| /camera_info                  | 相机参数      |

### 5.2 Yolov8 识别器

| topic                         | Description           |
| -----------                   | -----------           |
| /image                        | 原始图像              |
| /hbmem_img                    | 过程图像              |
| /dnn_node_sample              | 模型推理结果          |

____

## 6. 开发列表

- [x] Camera Node 25.1.8
- [x] Detector Node 25.2.6
- [x] Tracker Node 25.2.10
- [x] Unit Testing 25.2.15
- [x] YOLOv8 detection 25.2.20

____

## 7. 遵从协议  

大恒相机驱动采用 **commercial license**.  
传统识别部分采用 **MIT license**.  
Yolov8 识别采用 **Apache 2.0 license**.  

## 8. 致信我们

官网

>- Tianbot : <https://docs.tianbot.com/>
>- D-Robotics : <https://developer.d-robotics.cc/>
>- DAHENG IMAGING : <https://www.daheng-imaging.com/>

技术支持

>- D-Robotics : @wunuo
<https://github.com/wunuo1>
>- DAHENG IMAGING: @jerry

开发者邮件

>- BotAdv : <lenardo_smile@outlook.com>
