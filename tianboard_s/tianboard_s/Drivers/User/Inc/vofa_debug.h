#ifndef _VOFA_DEBUG_H
#define _VOFA_DEBUG_H

typedef struct{
    float fdata[9];
    const unsigned char tail[4];          /*尾帧 0x00, 0x00, 0x80, 0x7f*/ 
}Vofa_Send_Msg_t;

Vofa_Send_Msg_t Vofa_Msg = {.tail = {0x00, 0x00, 0x80, 0x7f}};

#endif
