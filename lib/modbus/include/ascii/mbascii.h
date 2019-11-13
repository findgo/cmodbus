
#ifndef __MB_ASCII_H
#define __MB_ASCII_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mbconfig.h"
#include "mbproto.h"

#include "mbcpu.h"
#include "mb.h"

/* ----------------------- Defines ------------------------------------------*/
#define MB_ASCII_DEFAULT_CR     '\r'    /*!< Default CR character for Modbus ASCII. */
#define MB_ASCII_DEFAULT_LF     '\n'    /*!< Default LF character for Modbus ASCII. */

typedef enum {
    STATE_ASCII_RX_IDLE,              /*!< ASCII Receiver is in idle state. */
    STATE_ASCII_RX_RCV,               /*!< ASCII Frame is beeing received. */
    STATE_ASCII_RX_WAIT_EOF,          /*!< ASCII Wait for End of Frame. */
    STATE_ASCII_TX_START,             /*!< ASCII Starting transmission (':' sent). */
    STATE_ASCII_TX_DATA,              /*!< ASCII Sending of data (Address, Data, LRC). */
    STATE_ASCII_TX_END,               /*!< ASCII End of transmission. */
    STATE_ASCII_TX_NOTIFY,            /*!< ASCII Notify sender that the frame has been sent. */
} MbASCIISndRcvState;

typedef enum {
    BYTE_HIGH_NIBBLE,           /*!< Character for high nibble of byte. */
    BYTE_LOW_NIBBLE             /*!< Character for low nibble of byte. */
} MbBytePos;

#if MB_ASCII_ENABLED > 0
#if MB_MASTER_ENABLED > 0

MbErrorCode_t MbmASCIIInit(MbsHandle_t dev, uint8_t ucPort,uint32_t ulBaudRate, MbParity_t eParity);
void MbmASCIIStart(MbsHandle_t dev);
void MbmASCIIStop(MbsHandle_t dev);
void MbmASCIIClose(MbsHandle_t dev);
MbReqResult_t MbmASCIIReceive(MbsHandle_t dev,MbHeader_t *phead,uint8_t *pfunCode, uint8_t **premain, uint16_t *premainLength);
MbReqResult_t MbmASCIISend(MbsHandle_t dev,const uint8_t *pAdu, uint16_t usAduLength);

void MbmASCIIReceiveFSM(MbsHandle_t dev);
void MbmASCIITransmitFSM(MbsHandle_t dev);
void MbmASCIITimerT1SExpired(MbsHandle_t dev);

#endif

#if MB_SLAVE_ENABLED > 0

MbErrorCode_t MbsASCIIInit(MbsHandle_t dev, uint8_t port, uint32_t baudRate, MbParity_t parity);

void MbsASCIIStart(MbsHandle_t dev);

void MbsASCIIStop(MbsHandle_t dev);

void MbsASCIIClose(MbsHandle_t dev);

MbErrorCode_t MbsASCIIReceiveParse(MbsHandle_t dev, MbsAduFrame_t *aduFrame);

MbErrorCode_t MbsASCIISend(MbsHandle_t dev, uint8_t slaveID, const uint8_t *pPdu, uint16_t len);

void MbsASCIIReceiveFSM(MbsHandle_t dev);

void MbsASCIITransmitFSM(MbsHandle_t dev);

void MbsASCIITimerT1SExpired(MbsHandle_t dev);

#endif
#endif

#ifdef __cplusplus
}
#endif

#endif

