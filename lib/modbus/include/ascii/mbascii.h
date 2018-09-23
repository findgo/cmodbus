
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

typedef enum
{
    STATE_ASCII_RX_IDLE,              /*!< ASCII Receiver is in idle state. */
    STATE_ASCII_RX_RCV,               /*!< ASCII Frame is beeing received. */
    STATE_ASCII_RX_WAIT_EOF,          /*!< ASCII Wait for End of Frame. */    
    STATE_ASCII_TX_START,             /*!< ASCII Starting transmission (':' sent). */
    STATE_ASCII_TX_DATA,              /*!< ASCII Sending of data (Address, Data, LRC). */
    STATE_ASCII_TX_END,               /*!< ASCII End of transmission. */
    STATE_ASCII_TX_NOTIFY,            /*!< ASCII Notify sender that the frame has been sent. */
} eMBASCIISndRcvState;

typedef enum
{
    BYTE_HIGH_NIBBLE,           /*!< Character for high nibble of byte. */
    BYTE_LOW_NIBBLE             /*!< Character for low nibble of byte. */
} eMBBytePos;


#if MB_ASCII_ENABLED > 0 &&  MB_MASTER_ENABLED > 0

MbErrorCode_t MbmASCIIInit(void *dev, uint8_t ucPort,uint32_t ulBaudRate, MbParity_t eParity);
void MbmASCIIStart(void *dev);
void MbmASCIIStop(void *dev);
void MbmASCIIClose(void *dev);
MbReqResult_t MbmASCIIReceive(void *dev,MbHeader_t *phead,uint8_t *pfunCode, uint8_t **premain, uint16_t *premainLength);
MbReqResult_t MbmASCIISend(void *dev,const uint8_t *pAdu, uint16_t usAduLength);

void MbmASCIIReceiveFSM(MbmDev_t *dev);
void MbmASCIITransmitFSM(MbmDev_t *dev);
void MbmASCIITimerT1SExpired(MbmDev_t *dev);

#endif

#if MB_ASCII_ENABLED > 0 &&  MB_SLAVE_ENABLED > 0
    
MbErrorCode_t MbsASCIIInit(void *dev, uint8_t ucPort,uint32_t ulBaudRate, MbParity_t eParity);
void MbsASCIIStart(void *dev);
void MbsASCIIStop(void *dev);
void MbsASCIIClose(void *dev);
MbErrorCode_t MbsASCIIReceive(void *dev, uint8_t *pucRcvAddress, uint8_t **pPdu,uint16_t *pusLength);
MbErrorCode_t MbsASCIISend(void *dev, uint8_t ucSlaveAddress, const uint8_t *pPdu, uint16_t usLength);

void MbsASCIIReceiveFSM(MbsDevice_t *dev);
void MbsASCIITransmitFSM(MbsDevice_t *dev);
void MbsASCIITimerT1SExpired(MbsDevice_t *dev);
    
#endif

#ifdef __cplusplus
}
#endif

#endif

