
#ifndef __MB_TCP_H
#define __MB_TCP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mbconfig.h"
#include "mbproto.h"
#include "mbframe.h"
#include "mbcpu.h"

#include "mb.h"

mb_ErrorCode_t eMBTCPInit(uint16_t ucTCPPort);
void vMBTCPStart(void *dev);
void vMBTCPStop(void *dev);
void vMBTCPClose(void *dev);
mb_ErrorCode_t eMBTCPReceive(void *dev, uint8_t *pucRcvAddress, uint8_t **pPdu,uint16_t *pusLength);
mb_ErrorCode_t eMBTCPSend(void *dev, uint8_t _unused, const uint8_t *pPdu,uint16_t usLength );

mb_ErrorCode_t eMBMasterTCPInit(uint16_t ucTCPPort);
void vMBMasterTCPStart(void *dev);
void vMBMasterTCPStop(void *dev);
void vMBMasterTCPClose(void *dev);
mb_ErrorCode_t eMBMasterTCPReceive(void *dev, uint8_t *pucRcvAddress, uint8_t **pPdu,uint16_t *pusLength);
mb_ErrorCode_t eMBMasterTCPSend(void *pdev,const uint8_t *pAdu, uint16_t usLength);


#ifdef __cplusplus
}
#endif
#endif
