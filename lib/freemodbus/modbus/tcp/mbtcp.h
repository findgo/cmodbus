
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

eMBErrorCode eMBTCPInit(uint16_t ucTCPPort);
void vMBTCPStart(void *dev);
void vMBTCPStop(void *dev);
void vMBTCPClose(void *dev);
eMBErrorCode eMBTCPReceive(void *dev, uint8_t *pucRcvAddress, uint8_t **pPdu,uint16_t *pusLength);
eMBErrorCode eMBTCPSend(void *dev, uint8_t _unused, const uint8_t *pPdu,uint16_t usLength );

#ifdef __cplusplus
}
#endif
#endif
