
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
typedef enum {
    STATE_RTU_RX_IDLE,              /*!< RTU Receiver is in idle state. */
    STATE_RTU_RX_RCV,               /*!< RTU Frame is beeing received. */
    STATE_RTU_TX_XMIT               /*!< RTU Transmitter is in transfer state. */
} MbRTUSndRcvState;

#if MB_RTU_ENABLED > 0
#if MB_MASTER_ENABLED > 0

MbErrorCode_t MbmRTUInit(MbmHandle_t dev, uint8_t ucPort, uint32_t ulBaudRate, MbParity_t eParity);

void MbmRTUStart(MbmHandle_t dev);

void MbmRTUStop(MbmHandle_t dev);

void MbmRTUClose(MbmHandle_t dev);

MbReqResult_t
MbmRTUReceive(MbmHandle_t dev, MbHeader_t *phead, uint8_t *pfunCode, uint8_t **premain, uint16_t *premainLength);

MbReqResult_t MbmRTUSend(MbmHandle_t dev, const uint8_t *pAdu, uint16_t usLength);

void MbmRTUReceiveFSM(MbmHandle_t dev);

void MbmRTUTransmitFSM(MbmHandle_t dev);

void MbmRTUTimerT15Expired(MbmHandle_t dev);

void MbmRTUTimerT35Expired(MbmHandle_t dev);

#endif

#if MB_SLAVE_ENABLED > 0

MbErrorCode_t MbsRTUInit(MbsHandle_t dev, uint8_t port, uint32_t baudRate, MbParity_t parity);

void MbsRTUStart(MbsHandle_t dev);

void MbsRTUStop(MbsHandle_t dev);

void MbsRTUClose(MbsHandle_t dev);

MbErrorCode_t MbsRTUReceiveParse(MbsHandle_t dev, MbsAduFrame_t *aduFrame);

MbErrorCode_t MbsRTUSend(MbsHandle_t dev, uint8_t slaveID, const uint8_t *pPdu, uint16_t len);

void MbsRTUReceiveFSM(MbsHandle_t dev);

void MbsRTUTransmitFSM(MbsHandle_t dev);

void MbsRTUTimerT15Expired(MbsHandle_t dev);

void MbsRTUTimerT35Expired(MbsHandle_t dev);

#endif
#endif

#ifdef __cplusplus
}
#endif

#endif
