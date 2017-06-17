
#ifndef __MB_ASCII_H
#define __MB_ASCII_H

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

#if MB_ASCII_ENABLED > 0
eMBErrorCode eMBASCIIInit(void *dev, uint8_t ucPort,uint32_t ulBaudRate, eMBParity eParity );
void vMBASCIIStart(void *dev,);
void vMBASCIIStop(void *dev);
void vMBASCIIClose(void *dev);
eMBErrorCode eMBASCIIReceive(void *dev, uint8_t *pucRcvAddress, uint8_t **pPdu,uint16_t *pusLength );
eMBErrorCode eMBASCIISend(void *dev, uint8_t ucSlaveAddress, const uint8_t *pPdu, uint16_t usLength );

bool xMBASCIIReceiveFSM(mb_device_t *dev);
bool xMBASCIITransmitFSM(mb_device_t *dev);
bool xMBASCIITimerT1SExpired(mb_device_t *dev);
#endif

#ifdef __cplusplus
}
#endif

#endif

