#ifndef _PROTOCOL_TASK_H_
#define _PROTOCOL_TASK_H_

#include "cmsis_os.h"
#include "protocol.h"
#include "stm32f4xx_hal.h"

#define PROTOCOL_MSG_LEN 512
#define PROTOCOL_MSG_QUENE_SIZE 8
#define PROTOCOL_TIMEOUT 500

#define USART1_TX_FINISH 0x01
#define USB_TX_FINISH 0x2
#define USB_TX_TIMEOUT 10
#define CONNECTED 0
#define DISCONNECTED 1

#define RADIAN_COEF 57.29578f

#define DEBUG_CMD_NAME_MAX_LEN 24
#define DEBUG_CMD_DESC_MAX_LEN 256

#define CFG_MAXARGS 16

#pragma pack(push)
#pragma pack(1)
struct ProtocolMsg
{
    uint8_t Msg[PROTOCOL_MSG_LEN];
    uint16_t MsgLen;
};

typedef struct
{
    char CmdName[DEBUG_CMD_NAME_MAX_LEN];
    void (*pCmdEntry)(int argc, char *argv[], char *ret, uint16_t max_ret_len);
    char Desc[DEBUG_CMD_DESC_MAX_LEN];
} DebugCmd_t;
#pragma pack(pop)

extern uint8_t ProtocolBuff[PROTOCOL_MSG_LEN];

extern osMailQId ProtocolRxMail;
extern osMailQId ProtocolTxMail;

extern osThreadId ProtocolSendTaskHandle;

#define ADD_DEBUG_CMD(cmd, func, desc) const DebugCmd_t Struct_##func __attribute__((section(".DEBUG_CMD_SECTOR"))) = {cmd, func, desc};

extern char DEBUG_CMD_SECTOR_Limit;
extern char DEBUG_CMD_SECTOR_Base;

void ProtocolTaskInit(void);
void ProtocolSend(uint16_t pack_type, uint8_t *msg, uint16_t len);
#endif
