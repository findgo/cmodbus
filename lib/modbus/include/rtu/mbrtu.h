
#ifndef _MB_RTU_H
#define _MB_RTU_H

#ifdef __cplusplus
extern "C" {
#endif
#include "mbconfig.h"
#include "mbproto.h"
#include "mbframe.h"

#include "mbcpu.h"
#include "mb.h"

/* ----------------------- Type definitions ---------------------------------*/
typedef enum
{
    STATE_RTU_RX_IDLE,              /*!< RTU Receiver is in idle state. */
    STATE_RTU_RX_RCV,               /*!< RTU Frame is beeing received. */
    STATE_RTU_TX_XMIT               /*!< RTU Transmitter is in transfer state. */
} eMBRTUSndRcvState;    

#if MB_RTU_ENABLED > 0 &&  MB_SLAVE_ENABLED > 0
mb_ErrorCode_t eMbsRTUInit(void *dev, uint8_t ucPort, uint32_t ulBaudRate,mb_Parity_t eParity);
void vMbsRTUStart(void *dev);
void vMbsRTUStop(void *dev);
void vMbsRTUClose(void *dev);
mb_ErrorCode_t eMbsRTUReceive(void *dev,uint8_t *pucRcvAddress, uint8_t **pPdu, uint16_t *pusLength);
mb_ErrorCode_t eMbsRTUSend(void *dev,uint8_t ucSlaveAddress, const uint8_t *pPdu, uint16_t usLength);

void vMbsRTUReceiveFSM(  mbs_Device_t *dev);
void vMbsRTUTransmitFSM(  mbs_Device_t *dev);
void vMBRTUTimerT15Expired(  mbs_Device_t *dev);
void vMbsRTUTimerT35Expired(  mbs_Device_t *dev);
#endif

#if MB_RTU_ENABLED > 0 &&  MB_MASTER_ENABLED > 0
mb_ErrorCode_t eMBMRTUInit(void *dev, uint8_t ucPort, uint32_t ulBaudRate, mb_Parity_t eParity);
void vMBMRTUStart(void *dev);
void vMBMRTUStop(void *dev);
void vMBMRTUClose(void *dev);
mb_reqresult_t eMBMRTUReceive(void *pdev,mb_header_t *phead,uint8_t *pfunCode, uint8_t **premain, uint16_t *premainLength);
mb_reqresult_t eMBMRTUSend(void *pdev,const uint8_t *pAdu, uint16_t usLength);

void vMBMRTUReceiveFSM(  mbm_Device_t *dev);
void vMBMRTUTransmitFSM(  mbm_Device_t *dev);
void vMBMRTUTimerT15Expired(  mbm_Device_t *dev);
void vMBMRTUTimerT35Expired(  mbm_Device_t *dev);
#endif

#ifdef __cplusplus
}
#endif

#endif
