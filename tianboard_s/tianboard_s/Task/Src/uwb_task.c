#include "uwb_task.h"
#include "uwb.h"
#include "cmsis_os.h"
#include "rc.h"
#include "rc_task.h"
#include "tim.h"
#include "protocol_task.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "can.h"
#include "can_wrapper.h"

osThreadId UwbFeedbackTaskHandle;

static void UwbFeedbackTaskEntry(void const *argument)
{
  struct uwb uwb;
  osDelay(1000);
  CanParamInit(&hcan2);
  for (;;)
  {
    osSignalWait(UWB_RX_DATA, osWaitForever);
    uwb.x_m = uwbInfo.coor_x/100.0f;
    uwb.y_m = uwbInfo.coor_y/100.0f;
    uwb.yaw = uwbInfo.yaw/100.0f/RADIAN_COEF;
/*    uwb.sig_level = uwbInfo.sig_level;
    if(uwb.sig_level == 1)
    {
      uwb.sig_level = 2;
    }
    else if(uwb.sig_level == 2)
    {
      uwb.sig_level = 1;
    }*/
    ProtocolSend(PACK_TYPE_UWB_RESPONSE, (uint8_t *)&uwb, sizeof(uwb));
  }
}
osThreadDef(UwbTask, UwbFeedbackTaskEntry, osPriorityAboveNormal, 0, 512);

void UwbTaskInit(void)
{
  UwbFeedbackTaskHandle = osThreadCreate(osThread(UwbTask), NULL);
}
