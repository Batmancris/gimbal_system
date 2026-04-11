# 常见问题 -- Fast Q & A

## 1. 相机相关

例：  
Q1：如何在ROS上配置相机环境？  
A1：官网下载Galaxy系列对应SDK，注意不同硬件系统对应不同版本。  
此外在集成进ROS系统时，还需要编写CMakelists.txt,声明依赖关系和编译流程，在packages.xml中声明依赖

_CMakelist.txt_  

```shell
target_link_libraries(${PROJECT_NAME}
  libgxiapi.so
  ${OpenCV_LIBS}
)
```

_packages.xml_  

```xml
  <depend>image_transport</depend>
  <depend>image_transport_plugins</depend>
  <depend>rm_utils</depend>
  <depend>camera_info_manager</depend>
```

## 2. 传统识别器相关

## 3. yolo识别器相关

## 4. 串口通信相关

## 5. 云台控制相关

项目维护人员会周期性汇总问题到Q&A中，如果遇到新问题请致信开发者
