# 上下位机通信说明

## 1. 当前采用的通信方式

- 物理链路：UART
- 上位机设备：RDK X5
- 下位机设备：DJI C 板 STM32F407
- 下位机串口：`USART1`
- 默认波特率：`921600`

## 2. 数据方向

当前主链路是单向下发：

- 上位机识别目标
- 上位机发送目标中心像素坐标
- 下位机接收后更新 `target_position`

## 3. 协议格式

总长度 8 字节：

```text
Byte0  Byte1  Byte2 Byte3 Byte4 Byte5 Byte6 Byte7
FA     FB     X_L   X_H   Y_L   Y_H   FC    FD
```

说明：

- `X_L`：X 低字节
- `X_H`：X 高字节
- `Y_L`：Y 低字节
- `Y_H`：Y 高字节
- 坐标按 `uint16_t` 发送

## 4. 下位机解析位置

下位机接收与解析在：

- `Gimbal control/Src/coordinate.c`

接收方式：

- DMA 接收
- 空闲中断判帧

解析成功后更新：

- `target_position.object_x`
- `target_position.object_y`
- `target_position.data_ready`

## 5. 上位机发送位置

上位机桥接节点在：

- `dev-branch/rm_gimbal_bridge/src/serial_bridge_node.cpp`

桥接节点做的事情：

1. 订阅 `/dnn_node_sample`
2. 选定一个目标
3. 取目标框中心点
4. 按 8 字节协议发送到串口

## 6. 当前限制

- 现在发送的是图像像素坐标，不是角度
- 现在还没有做下位机回传
- 现在还没有做 CRC/校验和
- 现在还没有做超时丢失保护协议

## 7. 后续建议

后续如果要提高稳定性，建议升级为：

1. 增加帧计数和校验位
2. 增加目标有效标志位
3. 发送相对中心偏差而不是原始像素
4. 再进一步升级为 yaw/pitch 角度误差
