/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "led_flow_task.h"
#include "chassis_task.h"
#include "detect_task.h"
#include "INS_task.h"
#include "gimbal_task.h"
#include "coordinate.h"
#include "target_state.h"
#include "vision_input.h"
#include "usb_cdc_test.h"
#include "remote_control.h"
#include "gimbal_behaviour.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
osThreadId gimbalTaskHandle;
osThreadId led_RGB_flow_handle;
osThreadId chassisTaskHandle;
osThreadId detect_handle;
osThreadId imuTaskHandle;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define VISION_DEBUG_PRINT 0
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
osThreadId testHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
static uint8_t vision_diag_checksum(const usb_vision_diag_frame_t *frame);
static void vision_diag_tick(uint32_t tick_ms);
static void cold_boot_led_diag_tick(uint32_t tick_ms);

/* USER CODE END FunctionPrototypes */

void test_task(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* GetTimerTaskMemory prototype (linked to static allocation support) */
void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/* USER CODE BEGIN GET_TIMER_TASK_MEMORY */
static StaticTask_t xTimerTaskTCBBuffer;
static StackType_t xTimerStack[configTIMER_TASK_STACK_DEPTH];

void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize )
{
  *ppxTimerTaskTCBBuffer = &xTimerTaskTCBBuffer;
  *ppxTimerTaskStackBuffer = &xTimerStack[0];
  *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
  /* place for user code */
}
/* USER CODE END GET_TIMER_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of test */
  osThreadDef(test, test_task, osPriorityNormal, 0, 128);
  testHandle = osThreadCreate(osThread(test), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  osThreadDef(gimbalTask, gimbal_task, osPriorityHigh, 0, 512);
	gimbalTaskHandle = osThreadCreate(osThread(gimbalTask), NULL);


  osThreadDef(imuTask, INS_task, osPriorityRealtime, 0, 1024);
  imuTaskHandle = osThreadCreate(osThread(imuTask), NULL);

  osThreadDef(ChassisTask, chassis_task, osPriorityAboveNormal, 0, 512);
  chassisTaskHandle = osThreadCreate(osThread(ChassisTask), NULL);

  osThreadDef(led, led_RGB_flow_task, osPriorityNormal, 0, 256);
  led_RGB_flow_handle = osThreadCreate(osThread(led), NULL);
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_test_task */
/**
  * @brief  Function implementing the test thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_test_task */
__weak void test_task(void const * argument)
{

  /* USER CODE BEGIN test_task */
  /* Infinite loop */
  
  for(;;)
  {
    uint32_t tick_ms = HAL_GetTick();
    const TargetPosition *pos = get_target_position();
    UsbCdcTest_HeartbeatTick(tick_ms);
    vision_diag_tick(tick_ms);
    cold_boot_led_diag_tick(tick_ms);
    if(pos->data_ready==1){
#if VISION_DEBUG_PRINT
      usart_printf("Value1: 0x%04X (%d), Value2: 0x%04X (%d)\r\n", pos->object_x, pos->object_x, pos->object_y, pos->object_y);
#endif
    }
    osDelay(10);
  }
  /* USER CODE END test_task */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
static uint8_t vision_diag_checksum(const usb_vision_diag_frame_t *frame)
{
  const uint8_t *bytes = (const uint8_t *)frame;
  uint8_t checksum = 0U;
  uint16_t i;

  if (frame == NULL)
  {
    return 0U;
  }

  for (i = 2U; i <= 44U; i++)
  {
    checksum ^= bytes[i];
  }

  return checksum;
}

static void vision_diag_tick(uint32_t tick_ms)
{
  static uint32_t last_diag_tick = 0U;
  usb_vision_diag_frame_t frame;
  gimbal_vision_diag_t diag;
  const vision_input_status_t *vision_status;

  if (last_diag_tick != 0U && (tick_ms - last_diag_tick) < 100U)
  {
    return;
  }

  if (UsbCdcTest_GetTxState() != 0)
  {
    return;
  }

  last_diag_tick = tick_ms;
  GimbalVisionDiag_Get(&diag);
  vision_status = VisionInput_GetStatus();

  frame.head0 = USB_VISION_DIAG_HEAD0;
  frame.head1 = USB_VISION_DIAG_HEAD1;
  frame.flags = 0U;
  if (diag.vision_enabled)
  {
    frame.flags |= USB_VISION_DIAG_FLAG_VISION_ENABLED;
  }
  if (diag.target_valid)
  {
    frame.flags |= USB_VISION_DIAG_FLAG_TARGET_VALID;
  }
  if (vision_status != NULL && vision_status->link_online)
  {
    frame.flags |= USB_VISION_DIAG_FLAG_LINK_ONLINE;
  }
  if (RC_data_is_error())
  {
    frame.flags |= USB_VISION_DIAG_FLAG_RC_ERROR;
  }
  if (toe_is_error(DBUS_TOE))
  {
    frame.flags |= USB_VISION_DIAG_FLAG_DBUS_TOE;
  }
  frame.seq = diag.target_seq;
  frame.raw_x = diag.raw_x;
  frame.raw_y = diag.raw_y;
  frame.error_x = diag.error_x;
  frame.error_y = diag.error_y;
  frame.yaw_add_mrad = diag.yaw_add_mrad;
  frame.pitch_add_mrad = diag.pitch_add_mrad;
  frame.parsed_frames = (uint16_t)(vision_status ? (vision_status->parsed_frames & 0xFFFFU) : 0U);
  frame.rx_bytes = (uint16_t)(vision_status ? (vision_status->rx_bytes & 0xFFFFU) : 0U);
  {
    const RC_ctrl_t *rc = get_remote_control_point();
    frame.rc_sw0 = (uint8_t)(rc ? rc->rc.s[0] : 0U);
    frame.rc_sw1 = (uint8_t)(rc ? rc->rc.s[1] : 0U);
    frame.rc_ch0 = (int16_t)(rc ? rc->rc.ch[0] : 0);
    frame.rc_ch1 = (int16_t)(rc ? rc->rc.ch[1] : 0);
    frame.rc_ch2 = (int16_t)(rc ? rc->rc.ch[2] : 0);
    frame.rc_ch3 = (int16_t)(rc ? rc->rc.ch[3] : 0);
  }
  frame.behaviour = (uint8_t)gimbal_behaviour_get();
  frame.manual_yaw_add_mrad = diag.manual_yaw_add_mrad;
  frame.manual_pitch_add_mrad = diag.manual_pitch_add_mrad;
  frame.yaw_mode = diag.yaw_mode;
  frame.pitch_mode = diag.pitch_mode;
  frame.yaw_set_mrad = diag.yaw_set_mrad;
  frame.pitch_set_mrad = diag.pitch_set_mrad;
  frame.yaw_given_current = diag.yaw_given_current;
  frame.pitch_given_current = diag.pitch_given_current;
  frame.checksum = 0U;
  frame.checksum = vision_diag_checksum(&frame);
  frame.tail0 = USB_VISION_DIAG_TAIL0;
  frame.tail1 = USB_VISION_DIAG_TAIL1;

  (void)UsbCdcTest_SendBytes((const uint8_t *)&frame, (uint16_t)sizeof(frame));
}


static void cold_boot_led_diag_tick(uint32_t tick_ms)
{
  static uint32_t last_parsed_frames = 0U;
  static uint32_t last_parsed_tick = 0U;
  const RC_ctrl_t *rc = get_remote_control_point();
  const vision_input_status_t *vision_status = VisionInput_GetStatus();
  gimbal_vision_diag_t diag;
  int8_t mode_sw = 0;
  uint16_t manual_active = 0U;
  uint16_t current_active = 0U;
  uint8_t upper_frame_recent = 0U;
  int16_t rc_yaw = 0;
  int16_t rc_pitch = 0;

  GimbalVisionDiag_Get(&diag);
  if (vision_status != NULL)
  {
    if (vision_status->parsed_frames != last_parsed_frames)
    {
      last_parsed_frames = vision_status->parsed_frames;
      last_parsed_tick = tick_ms;
    }
    if ((tick_ms - last_parsed_tick) < 500U && vision_status->parsed_frames > 0U)
    {
      upper_frame_recent = 1U;
    }
  }
  if (rc != NULL)
  {
    mode_sw = rc->rc.s[GIMBAL_MODE_CHANNEL];
    rc_yaw = rc->rc.ch[YAW_CHANNEL];
    rc_pitch = rc->rc.ch[PITCH_CHANNEL];
  }

  if (toe_is_error(DBUS_TOE))
  {
    led_debug_override_set(0xFFFF0000U, 200U);
    return;
  }

  if (RC_data_is_error())
  {
    led_debug_override_set(0xFFFFA000U, 250U);
    return;
  }

  if (toe_is_error(YAW_GIMBAL_MOTOR_TOE) || toe_is_error(PITCH_GIMBAL_MOTOR_TOE))
  {
    led_debug_override_set(0xFFFFFF00U, 120U);
    return;
  }

  {
    const error_t *errors = get_error_list_point();
    uint32_t now = xTaskGetTickCount();
    if (errors == NULL ||
        (now - errors[YAW_GIMBAL_MOTOR_TOE].work_time) < GIMBAL_MOTOR_READY_STABLE_MS ||
        (now - errors[PITCH_GIMBAL_MOTOR_TOE].work_time) < GIMBAL_MOTOR_READY_STABLE_MS)
    {
      led_debug_override_set(0xFFFFFF00U, 0U);
      return;
    }
  }

  manual_active = (uint16_t)(((rc_yaw > RC_DEADBAND) || (rc_yaw < -RC_DEADBAND) ||
                               (rc_pitch > RC_DEADBAND) || (rc_pitch < -RC_DEADBAND)) ? 1U : 0U);
  current_active = (uint16_t)(((diag.yaw_given_current > 300) || (diag.yaw_given_current < -300) ||
                               (diag.pitch_given_current > 300) || (diag.pitch_given_current < -300)) ? 1U : 0U);

  if (switch_is_down(mode_sw))
  {
    led_debug_override_set(0xFF0000FFU, 0U);
    return;
  }

  if (switch_is_mid(mode_sw))
  {
    if (gimbal_behaviour_get() != GIMBAL_RELATIVE_ANGLE)
    {
      led_debug_override_set(0xFFFF00FFU, 200U);
      return;
    }

    if (manual_active && !current_active)
    {
      led_debug_override_set(0xFFFFFFFFU, 80U);
      return;
    }

    if (manual_active && current_active)
    {
      led_debug_override_set(0xFF00FF00U, 80U);
      return;
    }

    led_debug_override_set(0xFF00FF00U, 0U);
    return;
  }

  if (switch_is_up(mode_sw))
  {
    if (diag.target_valid && current_active)
    {
      led_debug_override_set(0xFF00FFFFU, 80U);
      return;
    }

    if (diag.target_valid)
    {
      led_debug_override_set(0xFF00FFFFU, 0U);
      return;
    }

    if (upper_frame_recent)
    {
      led_debug_override_set(0xFFA0FFFFU, 0U);
      return;
    }

    if (diag.vision_enabled)
    {
      led_debug_override_set(0xFF00FFFFU, 250U);
    }
    else
    {
      led_debug_override_set(0xFFFFFFFFU, 150U);
    }
    return;
  }

  led_debug_override_set(0xFFFFA000U, 100U);
}

/* USER CODE END Application */


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
