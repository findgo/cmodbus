
#ifndef __MB_ASCII_H
#define __MB_ASCII_H

#ifdef __cplusplus
extern "C" {
#endif
#include "mbconfig.h"
#include "mbproto.h"
#include "mbframe.h"

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

mb_ErrorCode_t eMBMASCIIInit(void *dev, uint8_t ucPort,uint32_t ulBaudRate, mb_Parity_t eParity);
void vMBMASCIIStart(void *dev);
void vMBMASCIIStop(void *dev);
void vMBMASCIIClose(void *dev);
mb_reqresult_t eMBMASCIIReceive(void *dev,mb_header_t *phead,uint8_t *pfunCode, uint8_t **premain, uint16_t *premainLength);
mb_reqresult_t eMBMASCIISend(void *dev,const uint8_t *pAdu, uint16_t usAduLength);

void vMBMASCIIReceiveFSM(Mbm_Device_t *dev);
void vMBMASCIITransmitFSM(Mbm_Device_t *dev);
void vMBMASCIITimerT1SExpired(Mbm_Device_t *dev);

#endif

#if MB_ASCII_ENABLED > 0 &&  MB_SLAVE_ENABLED > 0
    
mb_ErrorCode_t eMbsASCIIInit(void *dev, uint8_t ucPort,uint32_t ulBaudRate, mb_Parity_t eParity);
void vMbsASCIIStart(void *dev);
void vMbsASCIIStop(void *dev);
void vMbsASCIIClose(void *dev);
mb_ErrorCode_t eMbsASCIIReceive(void *dev, uint8_t *pucRcvAddress, uint8_t **pPdu,uint16_t *pusLength);
mb_ErrorCode_t eMbsASCIISend(void *dev, uint8_t ucSlaveAddress, const uint8_t *pPdu, uint16_t usLength);

void vMbsASCIIReceiveFSM(Mbs_Device_t *dev);
void vMbsASCIITransmitFSM(Mbs_Device_t *dev);
void vMbsASCIITimerT1SExpired(Mbs_Device_t *dev);
    
#endif

#ifdef __cplusplus
}
#endif

#endif

