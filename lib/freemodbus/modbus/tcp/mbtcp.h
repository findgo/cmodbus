
#ifndef _MB_TCP_H
#define _MB_TCP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mbconfig.h"
#include "mbproto.h"
#include "mbframe.h"
#include "mbcpu.h"

#include "modbus.h"
#include "mbutils.h"

eMBErrorCode eMBTCPDoInit(uint16_t ucTCPPort);
void eMBTCPStart(mb_device_t *dev);
void eMBTCPStop(mb_device_t *dev);
void eMBTCPClose(mb_device_t *dev);
eMBErrorCode eMBTCPReceive(mb_device_t *dev, uint8_t *pucRcvAddress, uint8_t **pPdu,uint16_t *pusLength);
eMBErrorCode eMBTCPSend(mb_device_t *dev, uint8_t _unused, const uint8_t *pPdu,uint16_t usLength );

#ifdef __cplusplus
}
#endif
#endif
