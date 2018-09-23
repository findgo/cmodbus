
#ifndef _MB_RTU_H
#define _MB_RTU_H

#ifdef __cplusplus
extern "C" {
#endif
#include "mbconfig.h"
#include "mbproto.h"

#include "mbcpu.h"
#include "mb.h"

/* ----------------------- Type definitions ---------------------------------*/
typedef enum
{
    STATE_RTU_RX_IDLE,              /*!< RTU Receiver is in idle state. */
    STATE_RTU_RX_RCV,               /*!< RTU Frame is beeing received. */
    STATE_RTU_TX_XMIT               /*!< RTU Transmitter is in transfer state. */
} MbRTUSndRcvState;    

#if MB_RTU_ENABLED > 0 
#if MB_MASTER_ENABLED > 0

MbErrorCode_t MbmRTUInit(void *dev, uint8_t ucPort, uint32_t ulBaudRate, MbParity_t eParity);
void MbmRTUStart(void *dev);
void MbmRTUStop(void *dev);
void MbmRTUClose(void *dev);
MbReqResult_t MbmRTUReceive(void *dev,MbHeader_t *phead,uint8_t *pfunCode, uint8_t **premain, uint16_t *premainLength);
MbReqResult_t MbmRTUSend(void *dev,const uint8_t *pAdu, uint16_t usLength);

void MbmRTUReceiveFSM(  MbmDev_t *dev);
void MbmRTUTransmitFSM(  MbmDev_t *dev);
void MbmRTUTimerT15Expired(  MbmDev_t *dev);
void MbmRTUTimerT35Expired(  MbmDev_t *dev);

#endif

#if MB_SLAVE_ENABLED > 0

MbErrorCode_t MbsRTUInit(void *dev, uint8_t ucPort, uint32_t ulBaudRate,MbParity_t eParity);
void MbsRTUStart(void *dev);
void MbsRTUStop(void *dev);
void MbsRTUClose(void *dev);
MbErrorCode_t MbsRTUReceive(void *dev,uint8_t *pucRcvAddress, uint8_t **pPdu, uint16_t *pusLength);
MbErrorCode_t MbsRTUSend(void *dev,uint8_t ucSlaveAddress, const uint8_t *pPdu, uint16_t usLength);

void MbsRTUReceiveFSM(  MbsDev_t *dev);
void MbsRTUTransmitFSM(  MbsDev_t *dev);
void MbsRTUTimerT15Expired(  MbsDev_t *dev);
void MbsRTUTimerT35Expired(  MbsDev_t *dev);

#endif
#endif

#ifdef __cplusplus
}
#endif

#endif
