
#ifndef _MB_RTU_H
#define _MB_RTU_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mbconfig.h"
#include "mbproto.h"
#include "mbframe.h"
#include "mbcpu.h"

#include "modbus.h"
#include "mbutils.h"
#include "mbevent.h"

#include "port.h"

eMBErrorCode eMBRTUInit(void *dev, uint8_t ucPort, uint32_t ulBaudRate,eMBParity eParity);
void vMBRTUStart(void *dev);
void vMBRTUStop(void *dev);
void vMBRTUClose(void *dev);
eMBErrorCode eMBRTUReceive(void *dev,uint8_t *pucRcvAddress, uint8_t **pPdu, uint16_t *pusLength);
eMBErrorCode eMBRTUSend(void *dev,uint8_t ucSlaveAddress, const uint8_t *pPdu, uint16_t usLength);

bool xMBRTUReceiveFSM(  mb_device_t *dev);
bool xMBRTUTransmitFSM(  mb_device_t *dev);
bool xMBRTUTimerT15Expired(  mb_device_t *dev);
bool xMBRTUTimerT35Expired(  mb_device_t *dev);

#ifdef __cplusplus
}
#endif

#endif
