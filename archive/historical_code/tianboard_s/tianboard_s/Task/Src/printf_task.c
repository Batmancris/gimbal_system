#include "printf_task.h"
#include "vofa_debug.h"
#include "cmsis_os.h"
#include "usart.h"
#include "imuwit_task.h"
#include "imu_task.h"
#include "ackermann_task.h"
#include "odom.h"
#include "motor.h"

osThreadId PrintfTaskHandle;

void PrintfTaskEntry(void const *argument)
{
  /* USER CODE BEGIN ImuTaskEntry */
  /* Infinite loop */
  for (;;)
  {
    // Vofa_Msg.fdata[0] = bmi088_real_data.gyro[0];
    // Vofa_Msg.fdata[1] = bmi088_real_data.gyro[1];
    // Vofa_Msg.fdata[2] = bmi088_real_data.gyro[2];
    // Vofa_Msg.fdata[3] = bmi088_real_data.accel[0];
    // Vofa_Msg.fdata[4] = bmi088_real_data.accel[1];
    // Vofa_Msg.fdata[5] = bmi088_real_data.accel[2];
    // Vofa_Msg.fdata[6] = imu_angle[0];
    // Vofa_Msg.fdata[7] = imu_angle[1];
    // Vofa_Msg.fdata[8] = imu_angle[2];
    Vofa_Msg.fdata[0] = vescStatus.rpm;
    Vofa_Msg.fdata[1] = motor_rpm_set;
    Vofa_Msg.fdata[2] = pose.v_x_m;
    Vofa_Msg.fdata[3] = imu_angle[0];
    Vofa_Msg.fdata[4] = imu_angle[1];
    Vofa_Msg.fdata[5] = imu_angle[2];
    Vofa_Msg.fdata[6] = imuwit_angle[0];
    Vofa_Msg.fdata[7] = imuwit_angle[1];
    Vofa_Msg.fdata[8] = imuwit_angle[2];


    HAL_UART_Transmit_DMA(&huart6,(uint8_t*)&Vofa_Msg, sizeof(Vofa_Msg));
//		CDC_Transmit_FS((uint8_t*)&Vofa_Msg, sizeof(Vofa_Msg));
    osDelay(5);
  }
  /* USER CODE END ImuTaskEntry */
}

osThreadDef(PrintfTask, PrintfTaskEntry, osPriorityAboveNormal, 0, 512);

void PrintfTaskInit(void)
{
  PrintfTaskHandle = osThreadCreate(osThread(PrintfTask), NULL);
}
