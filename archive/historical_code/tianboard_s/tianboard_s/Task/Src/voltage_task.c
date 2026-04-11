#include "voltage_task.h"
#include "adc.h"
#include "main.h"
#include "stdio.h"
#include "stdlib.h"
#include "cmsis_os.h"
#include "stm32f4xx_hal.h"
#include "protocol_task.h"
#include "beep_task.h"

osThreadId VoltageFeedbackTaskHandle;

static void VoltageFeedbackTaskEntry(void const *argument)
{
  struct voltage voltage;
  // HAL_ADC_Start(&hadc1);
  // HAL_ADC_Start(&hadc3);
  for (;;)
  {
    uint32_t vrefint = 0;
    uint32_t vadc = 0;
    for (int i = 0; i < 128; i++)
    {
      HAL_ADC_Start(&hadc1);
      HAL_ADC_Start(&hadc3);
      vrefint += HAL_ADC_GetValue(&hadc1);
      vadc += HAL_ADC_GetValue(&hadc3);
    }
    vrefint = vrefint >> 8;
    vadc = vadc >> 8;
    voltage.Battery_voltage = vadc * 111 * 1.2 / vrefint / 11; // 分压电路111/11,内部参考电压1.219v
    ProtocolSend(PACK_TYPE_Voltage_RESPONSE, (uint8_t *)&voltage, sizeof(voltage));
    if(voltage.Battery_voltage < 10.8 && voltage.Battery_voltage > 10)
    {
      Beep(1, 100);
      Beep(0, 100);
      Beep(1, 100);
    }
    osDelay(1000);
  }
}

osThreadDef(VoltageTask, VoltageFeedbackTaskEntry, osPriorityAboveNormal, 0, 512);

void VoltageTaskInit(void)
{
  VoltageFeedbackTaskHandle = osThreadCreate(osThread(VoltageTask), NULL);
}