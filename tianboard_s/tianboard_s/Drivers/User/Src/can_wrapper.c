#include "can.h"
#include "can_wrapper.h"

void CanParamInit(CAN_HandleTypeDef *hcan)
{
    CAN_FilterTypeDef CAN_FilterConfigStructure;

    CAN_FilterConfigStructure.FilterMode = CAN_FILTERMODE_IDMASK;
    CAN_FilterConfigStructure.FilterScale = CAN_FILTERSCALE_32BIT;
    CAN_FilterConfigStructure.FilterIdHigh = 0x0000;
    CAN_FilterConfigStructure.FilterIdLow = 0x0000;
    CAN_FilterConfigStructure.FilterMaskIdHigh = 0x0000;
    CAN_FilterConfigStructure.FilterMaskIdLow = 0x0000;
    CAN_FilterConfigStructure.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    CAN_FilterConfigStructure.SlaveStartFilterBank = 14;

    if (hcan->Instance == CAN1)
    {
        CAN_FilterConfigStructure.FilterBank = 0; //can1(0-13)?can2(14-27)???????filter
    }
    else
    {
        CAN_FilterConfigStructure.FilterBank = 14; //can1(0-13)?can2(14-27)???????filter
    }
    CAN_FilterConfigStructure.FilterActivation = ENABLE;

    if (HAL_CAN_ConfigFilter(hcan, &CAN_FilterConfigStructure) != HAL_OK)
    {
        while (1)
            ;
    }

    if (HAL_CAN_Start(hcan) != HAL_OK)
    {
        while (1)
            ;
    }

    if (HAL_CAN_ActivateNotification(hcan, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK)
    {
        while (1)
            ;
    }
}

void CanSend(CAN_HandleTypeDef *hcan, uint16_t id, uint8_t *data, uint8_t len)
{
    CAN_TxHeaderTypeDef TxHeader;
    uint32_t TxMailBox;
    TxHeader.StdId = id;
    TxHeader.IDE = CAN_ID_STD;
    TxHeader.RTR = CAN_RTR_DATA;
    TxHeader.DLC = len;
    TxHeader.TransmitGlobalTime = DISABLE;
    HAL_CAN_AddTxMessage(hcan, &TxHeader, data, &TxMailBox);
}
