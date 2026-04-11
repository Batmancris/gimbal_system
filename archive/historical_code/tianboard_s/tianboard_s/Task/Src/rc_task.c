#include "rc_task.h"
#include "rc.h"
#include "cmsis_os.h"
#include "usart.h"
#include "stdlib.h"
#include "param.h"

osMailQId RcMail;
osMailQId CtrlMail;
osMailQDef(RcMail, RC_MSG_QUENE_SIZE, RcMsg_t);
osMailQDef(CtrlMail, CTRL_MSG_QUENE_SIZE, MotionCtrl_t);

osThreadId RcTaskHandle;

volatile uint32_t CtrlFlag = CTRL_TYPE_PC;
Dbus_RC_Ctl_t DbusCtrlData;
Sbus_RC_Ctl_t SbusCtrlData;
uint8_t RcBuff[RC_MSG_LEN];
static void RcTaskEntry(void const *argument)
{
  osEvent evt;
  RcMsg_t *p;
  float speed_level;
  osDelay(1000);

  HAL_UART_Receive_DMA(&huart3, RcBuff, RC_MSG_LEN);
  __HAL_UART_CLEAR_IDLEFLAG(&huart3);
  __HAL_UART_ENABLE_IT(&huart3, UART_IT_IDLE);
  /* Infinite loop */
  for (;;)
  {
    evt = osMailGet(RcMail, RC_TIMEOUT);
    if (evt.status == osEventMail)
    {
      p = evt.value.p;

      if (p->MsgLen == DBUS_RC_FRAME_LENGTH)
      {
        DbusRemoteDataProcess(p->Msg, &DbusCtrlData);
        if (DbusCtrlData.rc.ch0 != 0xF)
        {
          if (DbusCtrlData.rc.s1 == DBUS_RC_SW_UP)
          {
            CtrlFlag = CTRL_TYPE_PC;
          }
          else if (DbusCtrlData.rc.s1 == DBUS_RC_SW_DOWN)
          {
            CtrlFlag = CTRL_TYPE_MAG;
          }
          else
          {
            MotionCtrl_t *pMotionData = osMailAlloc(CtrlMail, osWaitForever);
            CtrlFlag = CTRL_TYPE_RC;
            if (abs(DbusCtrlData.rc.ch2 - param.rc_param.mid) < param.rc_param.dead_zone)
            {
              DbusCtrlData.rc.ch2 = param.rc_param.mid;
            }

            if (abs(DbusCtrlData.rc.ch3 - param.rc_param.mid) < param.rc_param.dead_zone)
            {
              DbusCtrlData.rc.ch3 = param.rc_param.mid;
            }

            if (abs(DbusCtrlData.rc.ch4 - param.rc_param.mid) < param.rc_param.dead_zone)
            {
              DbusCtrlData.rc.ch4 = param.rc_param.mid;
            }

            if (DbusCtrlData.rc.s2 == DBUS_RC_SW_UP)
            {
              speed_level = 1.0;
            }
            else if(DbusCtrlData.rc.s2 == DBUS_RC_SW_DOWN)
            {
              speed_level = 3.0;
            }
            else
            {
              speed_level = 2.0;
            }

            pMotionData->vx = (DbusCtrlData.rc.ch3 - param.rc_param.mid) * param.max_speed / speed_level / ((param.rc_param.max - param.rc_param.min) / 2);
            pMotionData->vy = -(DbusCtrlData.rc.ch2 - param.rc_param.mid) * param.max_speed / speed_level / ((param.rc_param.max - param.rc_param.min) / 2); // left is the positive direction for Y axis
            pMotionData->w = -(DbusCtrlData.rc.ch0 - param.rc_param.mid) * param.max_w / ((param.rc_param.max - param.rc_param.min) / 2);
            if (param.base_type == BASE_TYPE_ACKERMANN)
            {
              // steer_pwm_duty = (DbusCtrlData.rc.ch0 - 364) * (2000 - 1000) / (1648 - 364) + 1000; //364~1024~1648(rc)转到1000~1500~2000(steer)
              pMotionData->steering_angle = RcValue2AngleDeg(DbusCtrlData.rc.ch0);
            }						
            osMailPut(CtrlMail, pMotionData);
          }
        }
      }
      else if ((p->MsgLen == SBUS_RC_FRAME_LENGTH) && (p->Msg[0] == SBUS_START_FRAME))
      {
        SbusRemoteDataProcess(p->Msg, &SbusCtrlData);
        if (SbusCtrlData.rc_link)
        {
          if (SbusCtrlData.ch[6] != param.rc_param.max)
          {
            MotionCtrl_t *pMotionData = osMailAlloc(CtrlMail, osWaitForever);
            CtrlFlag = CTRL_TYPE_RC;
            if (abs(SbusCtrlData.ch[1] - param.rc_param.mid) < param.rc_param.dead_zone)
            {
              SbusCtrlData.ch[1] = param.rc_param.mid;
            }

            if (abs(SbusCtrlData.ch[2] - param.rc_param.mid) < param.rc_param.dead_zone)
            {
              SbusCtrlData.ch[2] = param.rc_param.mid;
            }

            if (abs(SbusCtrlData.ch[3] - param.rc_param.mid) < param.rc_param.dead_zone)
            {
              SbusCtrlData.ch[3] = param.rc_param.mid;
            }

            if (abs(SbusCtrlData.ch[4] - param.rc_param.mid) < param.rc_param.dead_zone)
            {
              SbusCtrlData.ch[4] = param.rc_param.mid;
            }

            if (abs(SbusCtrlData.ch[0] - param.rc_param.mid) < param.rc_param.dead_zone)
            {
              SbusCtrlData.ch[0] = param.rc_param.mid;
            }

            if (SbusCtrlData.ch[4] == param.rc_param.max)
            {
              speed_level = 1.0;
            }
            else if(SbusCtrlData.ch[4] == param.rc_param.min)
            {
              speed_level = 3.0;
            }
            else
            {
              speed_level = 2.0;
            }

            speed_level = LIMIT((float)(SbusCtrlData.ch[7] -  param.rc_param.min) / (float)(param.rc_param.max - param.rc_param.min), 0.1, 1.0);
            pMotionData->vx = (SbusCtrlData.ch[2] - param.rc_param.mid) * param.max_speed * speed_level / ((param.rc_param.max - param.rc_param.min) / 2);
            pMotionData->vy = -(SbusCtrlData.ch[3] - param.rc_param.mid) * param.max_speed * speed_level / ((param.rc_param.max - param.rc_param.min) / 2); // left is the positive direction for Y axis
            pMotionData->w = -(SbusCtrlData.ch[0] - param.rc_param.mid) * param.max_w / ((param.rc_param.max - param.rc_param.min) / 2);
            if (param.base_type == BASE_TYPE_ACKERMANN)
            {
              pMotionData->steering_angle = pMotionData->w * 57.29578f;
            }
            osMailPut(CtrlMail, pMotionData);
          }
          else
          {
            CtrlFlag = CTRL_TYPE_PC;
          }
        }
        else
        {
          CtrlFlag = CTRL_TYPE_PC;
        }
      }
      osMailFree(RcMail, p);
    }
    else if (evt.status == osEventTimeout)
    {
      if (CtrlFlag == CTRL_TYPE_RC)
      {
        CtrlFlag = CTRL_TYPE_PC;
      }
    }
  }
}
osThreadDef(RcTask, RcTaskEntry, osPriorityRealtime, 0, 512);
void RcTaskInit(void)
{
  RcMail = osMailCreate(osMailQ(RcMail), NULL);
  CtrlMail = osMailCreate(osMailQ(CtrlMail), NULL);

  RcTaskHandle = osThreadCreate(osThread(RcTask), NULL);
}
