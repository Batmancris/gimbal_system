#include "fw_update.h"
#include "rtc.h"
#include "beep.h"
#include "tim.h"

extern TIM_HandleTypeDef htim6;

static void HAL_DeInitTick(void)
{  
  HAL_TIM_Base_Stop_IT(&htim6);
  HAL_TIM_Base_DeInit(&htim6);
  HAL_NVIC_DisableIRQ(TIM6_DAC_IRQn); 
  __HAL_RCC_TIM6_CLK_DISABLE();
}

void CheckBoot(void)
{
    MX_RTC_Init();
    if (HAL_RTCEx_BKUPRead(&hrtc, 0) == BOOT_ROM_FLAG) //go bootrom
    {
        HAL_DeInitTick();
        HAL_RTCEx_BKUPWrite(&hrtc, 0, 0);
        HAL_RTC_MspDeInit(&hrtc);

        uint32_t IspSpInitVal;
        uint32_t IspJumpAddr;
        SYSCFG->MEMRMP = 0x01;
        void (*pIspFun)(void);
        IspSpInitVal = *(uint32_t *)0x1FFF0000;
        IspJumpAddr = *(uint32_t *)(0x1FFF0000 + 4);
        __set_MSP(IspSpInitVal);

        pIspFun = (void (*)(void))IspJumpAddr;
        (*pIspFun)();
    }
}

void FwUpdate(void)
{
    HAL_RTCEx_BKUPWrite(&hrtc, 0, BOOT_ROM_FLAG);
    HAL_NVIC_SystemReset();
}
