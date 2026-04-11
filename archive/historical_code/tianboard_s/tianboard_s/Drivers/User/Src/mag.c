#include "mag.h"

/**
**************************************************************************************************
* @Brief    Single byte data inversion        
* @Param    
*            @DesBuf: destination buffer
*            @SrcBuf: source buffer
* @RetVal    None
* @Note      (MSB)0101_0101 ---> 1010_1010(LSB)
**************************************************************************************************
*/
static void InvertUint8(unsigned char *DesBuf, unsigned char *SrcBuf)
{
    int i;
    unsigned char temp = 0;
    
    for(i = 0; i < 8; i++)
    {
        if(SrcBuf[0] & (1 << i))
        {
            temp |= 1<<(7-i);
        }
    }
    DesBuf[0] = temp;
}

/**
**************************************************************************************************
* @Brief    double byte data inversion        
* @Param    
*            @DesBuf: destination buffer
*            @SrcBuf: source buffer
* @RetVal    None
* @Note      (MSB)0101_0101_1010_1010 ---> 0101_0101_1010_1010(LSB)
**************************************************************************************************
*/
static void InvertUint16(unsigned short *DesBuf, unsigned short *SrcBuf)  
{  
    int i;  
    unsigned short temp = 0;    
    
    for(i = 0; i < 16; i++)  
    {  
        if(SrcBuf[0] & (1 << i))
        {          
            temp |= 1<<(15 - i);  
        }
    }  
    DesBuf[0] = temp;  
}

static unsigned short CRC16_MODBUS(unsigned char *puchMsg, unsigned int usDataLen)  
{  
    unsigned short wCRCin = 0xFFFF;  
    unsigned short wCPoly = 0x8005;  
    unsigned char wChar = 0;  
    
    while (usDataLen--)     
    {  
        wChar = *(puchMsg++);  
        InvertUint8(&wChar, &wChar);  
        wCRCin ^= (wChar << 8); 
        
        for(int i = 0; i < 8; i++)  
        {  
            if(wCRCin & 0x8000) 
            {
                wCRCin = (wCRCin << 1) ^ wCPoly;  
            }
            else  
            {
                wCRCin = wCRCin << 1; 
            }            
        }  
    }  
    InvertUint16(&wCRCin, &wCRCin);  
    return (wCRCin) ;  
} 

int MagDataProcess(uint8_t *pData, uint16_t *pMagInfo)
{
  uint16_t crc;
  if(pData == NULL)
  {
    return -1;
  }
  crc = CRC16_MODBUS(pData, MAG_MSG_LEN - 2);
  if(crc != (pData[MAG_MSG_LEN-1]*256 + pData[MAG_MSG_LEN-2]))
  {
    return -1;
  }
  if((pData[1] == 0xAB) && (pData[2] == 0x00) && (pData[3] == 0x28))
  {
    *pMagInfo = pData[4]*256 + pData[5];
  }
  return 0;
}
