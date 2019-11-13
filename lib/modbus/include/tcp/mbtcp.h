
#ifndef __MB_TCP_H
#define __MB_TCP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mbconfig.h"
#include "mbproto.h"
#include "mbcpu.h"

#include "mb.h"

MbErrorCode_t MbsTCPInit(uint16_t port);

void MbsTCPStart(void *dev);

void MbsTCPStop(void *dev);

void MbsTCPClose(void *dev);

MbErrorCode_t MbsTCPReceive(void *dev, uint8_t *pRcvAddress, uint8_t **pPdu, uint16_t *pLen);

MbErrorCode_t MbsTCPSend(void *dev, uint8_t _unused, const uint8_t *pPdu, uint16_t len);

MbErrorCode_t MbmTCPInit(uint16_t port);

void MbmTCPStart(void *dev);

void MbmTCPStop(void *dev);

void MbmTCPClose(void *dev);

MbErrorCode_t MbmTCPReceive(void *dev, uint8_t *pRcvAddress, uint8_t **pPdu, uint16_t *pLen);

MbErrorCode_t MbmTCPSend(void *pDev, const uint8_t *pAdu, uint16_t len);


#ifdef __cplusplus
}
#endif
#endif
