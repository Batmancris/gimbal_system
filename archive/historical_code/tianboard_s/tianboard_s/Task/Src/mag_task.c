#include "mag_task.h"
#include "rc_task.h"
#include "rc.h"
#include "cmsis_os.h"
#include "usart.h"
#include "stdlib.h"
#include "param.h"
#include "beep_task.h"
#include "mag.h"

osMailQId MagMail;
osMailQDef(MagMail, MAG_MSG_QUENE_SIZE, MagMsg_t);
MagMsg_t *pMagMsg;

#ifdef MAG_16_BIT
float MagNavTalbe[16] = {-8.0, -7.0, -6.0, -5.0, -4.0, -3.0, -2.0, -1.0 , 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};
#else
float MagNavTalbe[8] = {-4.0, -3.0, -2.0, -1.0 , 1.0, 2.0, 3.0, 4.0};
#endif

osThreadId MagTaskHandle;

uint8_t MagStatus = MAG_OUT;

static void BeepMagIn(void)
{
  Beep(0, 150);
  Beep(1, 150);
  Beep(2, 150);
}

static void BeepMagOut(void)
{
  Beep(2, 150);
  Beep(1, 150);
  Beep(0, 150);
}

static void MagTaskEntry(void const *argument)
{
  osEvent evt;
  MagMsg_t *p;
  uint16_t MagInfo;
  osDelay(5000);

  pMagMsg = osMailAlloc(MagMail, osWaitForever);
  // HAL_UART_Receive_DMA(&huart6, pMagMsg->Msg, MAG_MSG_BUFF_LEN);
  // __HAL_UART_CLEAR_IDLEFLAG(&huart6);
  // __HAL_UART_ENABLE_IT(&huart6, UART_IT_IDLE);
  /* Infinite loop */
  for (;;)
  {
    evt = osMailGet(MagMail, MAG_TIMEOUT);
    if (evt.status == osEventMail)
    {
      p = evt.value.p;
      if (CtrlFlag == CTRL_TYPE_MAG)
      {
        if(p->MsgLen == MAG_MSG_LEN)
        {
          if(MagDataProcess(p->Msg, &MagInfo) == 0)
          {
            if(MagInfo != 0)
            {
              if (MagStatus == MAG_OUT)
              {
                MagStatus = MAG_IN;
                BeepMagIn();
              }
            }
            else
            {
              if (MagStatus == MAG_IN)
              {
                MagStatus = MAG_OUT;
                BeepMagOut();
              }
            }
            
            MotionCtrl_t *pMotionData = osMailAlloc(CtrlMail, osWaitForever);
            if(MagStatus == MAG_OUT || MagInfo == 0xFFFF)
            {
              pMotionData->vx = 0;
              pMotionData->vy = 0;
              pMotionData->w = 0;
            }
            else
            {
              int i;
              int count = 0;
              float factor = 0;
              pMotionData->vx = param.mag_v;
              pMotionData->vy = 0;
            #ifdef MAG_16_BIT
              for(i=0;i<16;i++)
              {
                if((MagInfo & (1<<i)) != 0)
                {
                  factor += MagNavTalbe[i];
                  count ++;
                }
              }
              factor = factor/count/8.0f;
            #else
              for(i=0;i<8;i++)
              {
                if((MagInfo & (1<<i)) != 0)
                {
                  factor += MagNavTalbe[i];
                  count ++;
                }
              }
              factor = factor/count/4.0f;
            #endif
              pMotionData->w = factor * param.mag_max_w;
            }
            
            osMailPut(CtrlMail, pMotionData);
          }
        }
      }
      osMailFree(MagMail, p);
    }
    else if (evt.status == osEventTimeout)
    {
      if (CtrlFlag == CTRL_TYPE_MAG)
      {
        CtrlFlag = CTRL_TYPE_PC;
      }
    }
  }
}
osThreadDef(MagTask, MagTaskEntry, osPriorityHigh, 0, 512);

void MagTaskInit(void)
{
  MagMail = osMailCreate(osMailQ(MagMail), NULL);

  MagTaskHandle = osThreadCreate(osThread(MagTask), NULL);
}
