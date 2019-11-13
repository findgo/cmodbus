
#ifndef _MB_RTU_H
#define _MB_RTU_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mbconfig.h"
#include "mbproto.h"

#include "mbcpu.h"
#include "mb.h"

/**
* @defgroup rtu rtu实现
* @{
*/

/* ----------------------- Type definitions ---------------------------------*/
typedef enum {
    STATE_RTU_RX_IDLE,              /*!< RTU Receiver is in idle state. */
    STATE_RTU_RX_RCV,               /*!< RTU Frame is beeing received. */
    STATE_RTU_TX_XMIT               /*!< RTU Transmitter is in transfer state. */
} MbRTUSndRcvState;


MbErrCode_t MbmRTUInit(MbmHandle_t dev, uint8_t ucPort, uint32_t ulBaudRate, MbParity_t eParity);

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


/**
 * @brief modbus从机初始化
 *
 * @param dev: 句柄
 * @param port: 端口
 * @param baudRate: 波特率
 * @param parity: 校验位
 * @return ::MbErrCode_t
 */
MbErrCode_t MbsRTUInit(MbsHandle_t dev, uint8_t port, uint32_t baudRate, MbParity_t parity);

/**
 * @brief 启动
 *
 * @param dev: 句柄
 */
void MbsRTUStart(MbsHandle_t dev);
/**
 * @brief 停止
 * @param dev: 句柄
 */
void MbsRTUStop(MbsHandle_t dev);

void MbsRTUClose(MbsHandle_t dev);

MbErrCode_t MbsRTUReceiveParse(MbsHandle_t dev, MbsAduFrame_t *aduFrame);

MbErrCode_t MbsRTUSend(MbsHandle_t dev, uint8_t slaveID, const uint8_t *pPdu, uint16_t len);

void MbsRTUReceiveFSM(MbsHandle_t dev);

void MbsRTUTransmitFSM(MbsHandle_t dev);

void MbsRTUTimerT15Expired(MbsHandle_t dev);

void MbsRTUTimerT35Expired(MbsHandle_t dev);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
