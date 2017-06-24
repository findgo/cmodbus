
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


mb_ErrorCode_t eMBRTUInit(void *dev, uint8_t ucPort, uint32_t ulBaudRate,mb_Parity_t eParity);
void vMBRTUStart(void *dev);
void vMBRTUStop(void *dev);
void vMBRTUClose(void *dev);
mb_ErrorCode_t eMBRTUReceive(void *dev,uint8_t *pucRcvAddress, uint8_t **pPdu, uint16_t *pusLength);
mb_ErrorCode_t eMBRTUSend(void *dev,uint8_t ucSlaveAddress, const uint8_t *pPdu, uint16_t usLength);

void vMBRTUReceiveFSM(  mb_Device_t *dev);
void vMBRTUTransmitFSM(  mb_Device_t *dev);
void vMBRTUTimerT15Expired(  mb_Device_t *dev);
void vMBRTUTimerT35Expired(  mb_Device_t *dev);


mb_ErrorCode_t eMBMasterRTUInit(void *dev, uint8_t ucPort, uint32_t ulBaudRate, mb_Parity_t eParity);
void vMBMasterRTUStart(void *dev);
void vMBMasterRTUStop(void *dev);
void vMBMasterRTUClose(void *dev);
mb_ErrorCode_t eMBMasterRTUReceive(void *pdev,mb_header_t *phead,uint8_t *pfunCode, uint8_t **premain, uint16_t *premainLength);
mb_ErrorCode_t eMBMasterRTUSend(void *pdev,const uint8_t *pAdu, uint16_t usLength);

void vMBMasterRTUReceiveFSM(  mb_MasterDevice_t *dev);
void vMBMasterRTUTransmitFSM(  mb_MasterDevice_t *dev);
void vMBMasterRTUTimerT15Expired(  mb_MasterDevice_t *dev);
void vMBMasterRTUTimerT35Expired(  mb_MasterDevice_t *dev);

#ifdef __cplusplus
}
#endif

#endif
