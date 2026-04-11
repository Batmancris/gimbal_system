// IMU-WIT JY931
#include "cmsis_os.h"
#include "usart.h"
#include "imuwit_task.h"
#include <string.h>
#include "usart.h"
#include <math.h>

osThreadId ImuWitTaskHandle;
osThreadId ImuWitFeedbackTaskHandle;
osMailQId ImuWitMail;

osMailQDef(ImuWitMail, 8, ImuWitMsg_t);

uint8_t ImuWitBuff[ImuWit_MSG_LEN];

float imuwit_accel[3];  // Accelerometer data [ax, ay, az]
float imuwit_gyro[3];   // Gyroscope data [gx, gy, gz]
float imuwit_angle[3];  // Eular Angle [roll, pitch, yaw]
float imuwit_quat[4];

void angle2quat(const float angle[3], float quat[4]) {
    float cy = cosf(angle[2] * 0.5f);
    float sy = sinf(angle[2] * 0.5f);
    float cp = cosf(angle[1] * 0.5f);
    float sp = sinf(angle[1] * 0.5f);
    float cr = cosf(angle[0] * 0.5f);
    float sr = sinf(angle[0] * 0.5f);

    quat[0] = cr * cp * cy + sr * sp * sy; // w
    quat[1] = sr * cp * cy - cr * sp * sy; // x
    quat[2] = cr * sp * cy + sr * cp * sy; // y
    quat[3] = cr * cp * sy - sr * sp * cy; // z

    float norm = sqrtf(quat[0]*quat[0] + quat[1]*quat[1] + quat[2]*quat[2] + quat[3]*quat[3]);
    quat[0] /= norm;
    quat[1] /= norm;
    quat[2] /= norm;
    quat[3] /= norm;
}

uint8_t calc_sumcrc(uint8_t *data, int len) {
    uint8_t sum = 0;
    for(int i = 0; i < len; i++) {
        sum += data[i];
    }
    return sum;
}

void parse_frame(uint8_t *frame) {
    if(frame[0] != 0x55) {
        return;
    }

    uint8_t type = frame[1];
    uint8_t sumcrc = frame[10];
    if (calc_sumcrc(frame, 10) != sumcrc) {
        return;
    }

    switch(type) {
        case 0x51: {  // Accelerometer data
            int16_t ax = (int16_t)((frame[3] << 8) | frame[2]);
            int16_t ay = (int16_t)((frame[5] << 8) | frame[4]);
            int16_t az = (int16_t)((frame[7] << 8) | frame[6]);
            imuwit_accel[0] = ax / 32768.0 * 16.0;  // ax in g
            imuwit_accel[1] = ay / 32768.0 * 16.0;  // ay in g
            imuwit_accel[2] = az / 32768.0 * 16.0;  // az in g
            break;
        }
        case 0x52: {  // Gyroscope data
            int16_t gx = (int16_t)((frame[3] << 8) | frame[2]);
            int16_t gy = (int16_t)((frame[5] << 8) | frame[4]);
            int16_t gz = (int16_t)((frame[7] << 8) | frame[6]);
            imuwit_gyro[0] = gx / 32768.0 * 2000.0;  // gx in dps
            imuwit_gyro[1] = gy / 32768.0 * 2000.0;  // gy in dps
            imuwit_gyro[2] = gz / 32768.0 * 2000.0;  // gz in dps
            break;
        }
        case 0x53: {  // Eular Angle data
            int16_t roll  = (int16_t)((frame[3] << 8) | frame[2]);
            int16_t pitch = (int16_t)((frame[5] << 8) | frame[4]);
            int16_t yaw   = (int16_t)((frame[7] << 8) | frame[6]);
            imuwit_angle[0] = roll / 32768.0 * PI ;  // roll in rad
            imuwit_angle[1] = pitch / 32768.0 * PI; // pitch in rad
            imuwit_angle[2] = yaw / 32768.0 * PI;   // yaw in rad
            angle2quat(imuwit_angle,imuwit_quat);
            break;
        }
        default:
            break;
    }
}

static void ImuWitProcess(uint8_t *Buf, uint8_t Len)
{
    for (int i = 0; i <= Len - 11; i++) {
        if (Buf[i] == 0x55) {
            parse_frame(&Buf[i]);
            i += 10;  // skip 11 bytes of the current frame
        }
    }
}

uint8_t unlock_cmd[] = {0xFF, 0xAA, 0x69, 0x88, 0xB5};
uint8_t calibrate_cmd[] = {0xFF, 0xAA, 0x01, 0x08, 0x00};
uint8_t save_cmd[] = {0xFF, 0xAA, 0x00, 0x00, 0x00};

void send_command_with_delay(uint8_t *command, uint16_t size, uint16_t delay)
{
    HAL_UART_Transmit_DMA(&huart1, command, size);
    osDelay(delay);
}

void ImuWitEntry(void const *argument)
{
    osEvent evt;
    ImuWitMsg_t *p;
    send_command_with_delay(unlock_cmd, sizeof(unlock_cmd), 200); // 1. 解锁命令
    send_command_with_delay(calibrate_cmd, sizeof(calibrate_cmd), 3000); // 2. 参考角度归零命令
    send_command_with_delay(save_cmd, sizeof(save_cmd), 100);  // 3. 保存命令
    HAL_UART_Receive_DMA(&huart1, ImuWitBuff, ImuWit_MSG_LEN);
    __HAL_UART_CLEAR_IDLEFLAG(&huart1);
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
    /* Infinite loop */
    for (;;)
    {
        evt = osMailGet(ImuWitMail, 500);
        if (evt.status == osEventMail)
        {
            p = evt.value.p;
            ImuWitProcess(p->Msg, p->MsgLen);
            osMailFree(ImuWitMail, p);
        }
    }
}

#include "protocol_task.h"
void ImuWitFeedbackTaskEntry(void const *argument)
{
  /* USER CODE BEGIN ImuTaskEntry */
//   struct imu_feedback imu_feedback;
  osDelay(1000);
  /* Infinite loop */
  for (;;)
  {
//    imu_feedback.quat.w = imuwit_quat[0];
//    imu_feedback.quat.x = imuwit_quat[1];
//    imu_feedback.quat.y = imuwit_quat[2];
//    imu_feedback.quat.z = imuwit_quat[3];
//    imu_feedback.linear_acc.x = imuwit_accel[0];
//    imu_feedback.linear_acc.y = imuwit_accel[1];
//    imu_feedback.linear_acc.z = imuwit_accel[2];
//    imu_feedback.angular_vel.x = imuwit_gyro[0];
//    imu_feedback.angular_vel.y = imuwit_gyro[1];
//    imu_feedback.angular_vel.z = imuwit_gyro[2];
//    ProtocolSend(PACK_TYPE_IMU_REPONSE, (uint8_t *)&imu_feedback, sizeof(struct imu_feedback));
    osDelay(1);
  }
  /* USER CODE END ImuTaskEntry */
}

osThreadDef(ImuWitTask, ImuWitEntry, osPriorityAboveNormal, 0, 512);
osThreadDef(ImuWitFeedbackTask, ImuWitFeedbackTaskEntry, osPriorityAboveNormal, 0, 512);

void ImuWitTaskInit(void)
{
    ImuWitMail = osMailCreate(osMailQ(ImuWitMail), NULL);
    ImuWitTaskHandle = osThreadCreate(osThread(ImuWitTask), NULL);
    ImuWitFeedbackTaskHandle = osThreadCreate(osThread(ImuWitFeedbackTask), NULL);
}
