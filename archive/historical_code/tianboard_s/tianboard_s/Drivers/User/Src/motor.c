#include "motor.h"
#include "can.h"
#include "param.h"
#include "string.h"
#include "uwb.h"
#include "uwb_task.h"

#define CAN_RX_BUFF_SIZE 8

volatile MotorInfo_t motorInfo[8];
volatile MotecMotorInfo_t motecMotorInfo[2];
volatile VescStatus_t vescStatus;
volatile UwbInfo_t uwbInfo;
volatile uint8_t motecCtrlState;

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef RxHeader;
    char CanRxData[CAN_RX_BUFF_SIZE];
    if (hcan->Instance == CAN1)
    {
        if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &RxHeader, (uint8_t *)CanRxData) != HAL_OK)
        {
            /* Reception Error */
            Error_Handler();
        }
        if (param.motor_type == MOTOR_VESC6_CAN)
        {
            switch (RxHeader.ExtId)
            {
            case ((CAN_PACKET_STATUS << 8) | (VESC_ID)):
            {
                vescStatus.rpm = (CanRxData[0] << 24) | (CanRxData[1] << 16) | (CanRxData[2] << 8) | CanRxData[3];
                vescStatus.current = (CanRxData[4] << 8) | CanRxData[5] / 10;
                vescStatus.duty = (CanRxData[6] << 8) | CanRxData[7] / 100;
                break;
            }
            default:
                break;
            }
        }
        else if (param.motor_type == MOTOR_DJI_CAN)
        {
            if (((RxHeader.StdId > 0x200) && (RxHeader.StdId < 0x205)) && (RxHeader.IDE == CAN_ID_STD) && (RxHeader.DLC == 8))
            {
                motorInfo[RxHeader.StdId - 0x200 - 1].position = (CanRxData[0] << 8) | CanRxData[1];
                motorInfo[RxHeader.StdId - 0x200 - 1].w = ((CanRxData[2] << 8) | CanRxData[3]);
                motorInfo[RxHeader.StdId - 0x200 - 1].current = ((CanRxData[4] << 8) | CanRxData[5]);
                motorInfo[RxHeader.StdId - 0x200 - 1].temperature = CanRxData[6];
            }
        }
        else if (param.motor_type == MOTOR_MOTEC_CAN)
        {
            if (RxHeader.StdId == 0x701)
            {
                motecCtrlState = CanRxData[0];
            }
            if ((RxHeader.StdId == 0x181) || (RxHeader.StdId == 0x182))
            {
                memcpy((void *)&motecMotorInfo[RxHeader.StdId - 0x181], CanRxData, 8);
            }
        }
    }
    else
    {
        if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &RxHeader, (uint8_t *)CanRxData) != HAL_OK)
        {
            /* Reception Error */
            Error_Handler();
        }

        static int uwb_flag = -1;
        static int offset = 0;
        static int first_flag = 0;
        if (RxHeader.StdId == 0x259)
        {
            switch (uwb_flag)
            {
            case 0:
                memcpy(((uint8_t *)(&uwbInfo)) + offset, CanRxData, RxHeader.DLC);
                offset += RxHeader.DLC;
                uwb_flag++;
                break;

            case 1:
                memcpy(((uint8_t *)(&uwbInfo)) + offset, CanRxData, RxHeader.DLC);
                offset += RxHeader.DLC;
                uwb_flag++;
                break;

            case 2:
                memcpy(((uint8_t *)(&uwbInfo)) + offset, CanRxData, RxHeader.DLC);
                offset += RxHeader.DLC;
                uwb_flag++;
                break;

            default:
                uwb_flag = -1;
                offset = 0;
                break;
            }
            if (RxHeader.DLC == 6)
            {
                uwb_flag = 0;
                offset = 0;
                if (!first_flag)
                {
                    first_flag++;
                }
                else
                {
                    osSignalSet(UwbFeedbackTaskHandle, UWB_RX_DATA);
                }
            }
        }
    }
}

void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef RxHeader;
    char CanRxData[CAN_RX_BUFF_SIZE];
    if (hcan->Instance == CAN1)
    {
        if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO1, &RxHeader, (uint8_t *)CanRxData) != HAL_OK)
        {
            /* Reception Error */
            Error_Handler();
        }
        if (param.motor_type == MOTOR_DJI_CAN)
        {
            if (((RxHeader.StdId > 0x200) && (RxHeader.StdId < 0x205)) && (RxHeader.IDE == CAN_ID_STD) && (RxHeader.DLC == 8))
            {
                motorInfo[RxHeader.StdId - 0x200 - 1].position = (CanRxData[0] << 8) | CanRxData[1];
                motorInfo[RxHeader.StdId - 0x200 - 1].w = ((CanRxData[2] << 8) | CanRxData[3]);
                motorInfo[RxHeader.StdId - 0x200 - 1].current = ((CanRxData[4] << 8) | CanRxData[5]);
                motorInfo[RxHeader.StdId - 0x200 - 1].temperature = CanRxData[6];
            }
        }
        else if (param.motor_type == MOTOR_MOTEC_CAN)
        {
            if (RxHeader.StdId == 0x701)
            {
                motecCtrlState = CanRxData[0];
            }
            if ((RxHeader.StdId == 0x181) || (RxHeader.StdId == 0x182))
            {
                memcpy((void *)&motecMotorInfo[RxHeader.StdId - 0x181], CanRxData, 8);
            }
        }
    }
    else
    {
        if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO1, &RxHeader, (uint8_t *)CanRxData) != HAL_OK)
        {
            /* Reception Error */
            Error_Handler();
        }

        static int uwb_flag = -1;
        static int offset = 0;
        static int first_flag = 0;
        if (RxHeader.StdId == 0x259)
        {
            switch (uwb_flag)
            {
            case 0:
                memcpy(((uint8_t *)(&uwbInfo)) + offset, CanRxData, RxHeader.DLC);
                offset += RxHeader.DLC;
                uwb_flag++;
                break;

            case 1:
                memcpy(((uint8_t *)(&uwbInfo)) + offset, CanRxData, RxHeader.DLC);
                offset += RxHeader.DLC;
                uwb_flag++;
                break;

            case 2:
                memcpy(((uint8_t *)(&uwbInfo)) + offset, CanRxData, RxHeader.DLC);
                offset += RxHeader.DLC;
                uwb_flag++;
                break;

            default:
                uwb_flag = -1;
                offset = 0;
                break;
            }
            if (RxHeader.DLC == 6)
            {
                uwb_flag = 0;
                offset = 0;
                if (!first_flag)
                {
                    first_flag++;
                }
                else
                {
                    osSignalSet(UwbFeedbackTaskHandle, UWB_RX_DATA);
                }
            }
        }
    }
}

void CAN1_Send_Msg(uint8_t id, uint32_t packet_id, uint8_t *msg, uint8_t len)
{
    CAN_TxHeaderTypeDef TxHeader;
    uint32_t TxMailbox;

    // 设置扩展帧标识符（29位）
    TxHeader.StdId = 0x00;                // 标准帧ID（未使用）
    TxHeader.ExtId = packet_id << 8 | id; // 扩展帧ID
    TxHeader.IDE = CAN_ID_EXT;            // 使用扩展帧（CAN_ID_EXT）或标准帧（CAN_ID_STD）
    TxHeader.RTR = CAN_RTR_DATA;          // 数据帧（CAN_RTR_REMOTE 远程帧）
    TxHeader.DLC = len;                   // 数据长度（最大8字节）

    HAL_CAN_AddTxMessage(&hcan1, &TxHeader, msg, &TxMailbox);
}

void SetVescDuty(float duty)
{
    int32_t send_duty = (int32_t)(duty * 100000);
    uint8_t msg[4];

    // 按字节拆分数据
    msg[0] = (send_duty >> 24) & 0xFF;
    msg[1] = (send_duty >> 16) & 0xFF;
    msg[2] = (send_duty >> 8) & 0xFF;
    msg[3] = (send_duty) & 0xFF;

    CAN1_Send_Msg(VESC_ID, CAN_PACKET_SET_DUTY, msg, sizeof(msg));
}

void SetVescRPM(int32_t rpm)
{
    uint8_t msg[4];

    // 按字节拆分数据
    msg[0] = (rpm >> 24) & 0xFF;
    msg[1] = (rpm >> 16) & 0xFF;
    msg[2] = (rpm >> 8) & 0xFF;
    msg[3] = (rpm) & 0xFF;

    CAN1_Send_Msg(VESC_ID, CAN_PACKET_SET_RPM, msg, sizeof(msg));

    if (rpm == 0)
    {
        float brake_rel = 0.9f;
        int32_t send_val = (int32_t)(brake_rel * 100000.0f);
        msg[0] = (send_val >> 24) & 0xFF;
        msg[1] = (send_val >> 16) & 0xFF;
        msg[2] = (send_val >> 8) & 0xFF;
        msg[3] = (send_val) & 0xFF;
        CAN1_Send_Msg(VESC_ID, CAN_PACKET_SET_CURRENT_BRAKE_REL, msg, sizeof(msg));
    }
}

void SetCanMotor(int16_t iq1, int16_t iq2, int16_t iq3, int16_t iq4, CAN_HandleTypeDef *can)
{
    CAN_TxHeaderTypeDef TxHeader;
    uint32_t TxMailBox;
    uint8_t Data[8];
    TxHeader.StdId = 0x200;
    TxHeader.IDE = CAN_ID_STD;
    TxHeader.RTR = CAN_RTR_DATA;
    TxHeader.DLC = 0x08;
    TxHeader.TransmitGlobalTime = DISABLE;
    Data[0] = (iq1 >> 8);
    Data[1] = iq1;
    Data[2] = (iq2 >> 8);
    Data[3] = iq2;
    Data[4] = iq3 >> 8;
    Data[5] = iq3;
    Data[6] = iq4 >> 8;
    Data[7] = iq4;
    HAL_CAN_AddTxMessage(can, &TxHeader, Data, &TxMailBox);
}

void MotecEnterOpMode(void)
{
    while (1)
    {
        if (motecCtrlState == 0x05)
        {
            break;
        }
        else if (motecCtrlState == 0x7F)
        {
            CAN_TxHeaderTypeDef TxHeader;
            uint32_t TxMailBox;
            uint8_t Data[2];
            TxHeader.StdId = 0;
            TxHeader.IDE = CAN_ID_STD;
            TxHeader.RTR = CAN_RTR_DATA;
            TxHeader.DLC = 0x02;
            TxHeader.TransmitGlobalTime = DISABLE;
            Data[0] = 0x01;
            Data[1] = 0x00;
            HAL_CAN_AddTxMessage(MOTEC_CAN_NUM, &TxHeader, Data, &TxMailBox);
        }
        osDelay(10);
    }
}

void MotecPowerOn(int motor)
{
    CAN_TxHeaderTypeDef TxHeader;
    uint32_t TxMailBox;
    uint8_t Data[2];
    TxHeader.StdId = 0x201 + motor;
    TxHeader.IDE = CAN_ID_STD;
    TxHeader.RTR = CAN_RTR_DATA;
    TxHeader.DLC = 0x02;
    TxHeader.TransmitGlobalTime = DISABLE;
    Data[0] = 0x06;
    Data[1] = 0x00;
    HAL_CAN_AddTxMessage(MOTEC_CAN_NUM, &TxHeader, Data, &TxMailBox);
}

void MotecEnMotor(int motor)
{
    CAN_TxHeaderTypeDef TxHeader;
    uint32_t TxMailBox;
    uint8_t Data[2];
    TxHeader.StdId = 0x201 + motor;
    TxHeader.IDE = CAN_ID_STD;
    TxHeader.RTR = CAN_RTR_DATA;
    TxHeader.DLC = 0x02;
    TxHeader.TransmitGlobalTime = DISABLE;
    Data[0] = 0x07;
    Data[1] = 0x00;
    HAL_CAN_AddTxMessage(MOTEC_CAN_NUM, &TxHeader, Data, &TxMailBox);
}

void MotecSetSpeed(int16_t rpm, int16_t motor)
{
    CAN_TxHeaderTypeDef TxHeader;
    uint32_t TxMailBox;
    uint8_t Data[8];
    TxHeader.StdId = 0x201 + motor;
    TxHeader.IDE = CAN_ID_STD;
    TxHeader.RTR = CAN_RTR_DATA;
    TxHeader.DLC = 0x04;
    TxHeader.TransmitGlobalTime = DISABLE;
    Data[0] = 0x0F;
    Data[1] = 0x00;
    Data[2] = rpm & 0xFF;
    Data[3] = rpm >> 8;
    HAL_CAN_AddTxMessage(MOTEC_CAN_NUM, &TxHeader, Data, &TxMailBox);
}

void MotecInit(int motor)
{
    int motecInitStep = 0;
    int timeout = 0;
    while (1)
    {
        switch (motecInitStep)
        {
        case 0:
            MotecPowerOn(motor);
            timeout = 0;
            motecInitStep++;
            break;
        case 1:
            if ((motecMotorInfo[motor].status & 0xFF) == 0x31)
            {
                motecInitStep++;
            }
            else
            {
                timeout++;
            }
            if (timeout >= MOTEC_CAN_OP_TIMEOUT)
            {
                motecInitStep--;
            }
            break;
        case 2:
            MotecEnMotor(motor);
            timeout = 0;
            motecInitStep++;
            break;
        case 3:
            if ((motecMotorInfo[motor].status & 0xFF) == 0x33)
            {
                return;
            }
            else
            {
                timeout++;
            }
            if (timeout >= MOTEC_CAN_OP_TIMEOUT)
            {
                motecInitStep--;
            }
        default:
            break;
        }
    }
}