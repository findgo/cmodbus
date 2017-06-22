
#ifndef _MB_RTU_H
#define _MB_RTU_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mbcpu.h"
#include "modbus.h"
    
mb_ErrorCode_t eMBRTUInit(void *dev, uint8_t ucPort, uint32_t ulBaudRate,mb_Parity_t eParity);
void vMBRTUStart(void *dev);
void vMBRTUStop(void *dev);
void vMBRTUClose(void *dev);
mb_ErrorCode_t eMBRTUReceive(void *dev,uint8_t *pucRcvAddress, uint8_t **pPdu, uint16_t *pusLength);
mb_ErrorCode_t eMBRTUSend(void *dev,uint8_t ucSlaveAddress, const uint8_t *pPdu, uint16_t usLength);

bool xMBRTUReceiveFSM(  mb_Device_t *dev);
bool xMBRTUTransmitFSM(  mb_Device_t *dev);
bool xMBRTUTimerT15Expired(  mb_Device_t *dev);
bool xMBRTUTimerT35Expired(  mb_Device_t *dev);

mb_ErrorCode_t eMBMasterRTUInit(void *dev, uint8_t ucPort, uint32_t ulBaudRate, mb_Parity_t eParity);
void vMBMasterRTUStart(void *dev);
void vMBMasterRTUStop(void *dev);
void vMBMasterRTUClose(void *dev);
mb_ErrorCode_t eMBMasterRTUReceive(void *pdev,mb_header_t *phead,uint8_t *pfunCode, uint8_t **premain, uint16_t *premainLength);
mb_ErrorCode_t eMBMasterRTUSend(void *pdev,const uint8_t *pAdu, uint16_t usLength);
bool xMBMasterRTUReceiveFSM(  mb_MasterDevice_t *dev);
bool xMBMasterRTUTransmitFSM(  mb_MasterDevice_t *dev);
bool xMBMasterRTUTimerT15Expired(  mb_MasterDevice_t *dev);
bool xMBMasterRTUTimerT35Expired(  mb_MasterDevice_t *dev);

#ifdef __cplusplus
}
#endif

#endif
