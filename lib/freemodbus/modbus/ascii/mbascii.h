
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

mb_ErrorCode_t eMBASCIIInit(void *dev, uint8_t ucPort,uint32_t ulBaudRate, mb_Parity_t eParity);
void vMBASCIIStart(void *dev);
void vMBASCIIStop(void *dev);
void vMBASCIIClose(void *dev);
mb_ErrorCode_t eMBASCIIReceive(void *dev, uint8_t *pucRcvAddress, uint8_t **pPdu,uint16_t *pusLength);
mb_ErrorCode_t eMBASCIISend(void *dev, uint8_t ucSlaveAddress, const uint8_t *pPdu, uint16_t usLength);

void vMBASCIIReceiveFSM(mb_Device_t *dev);
void vMBASCIITransmitFSM(mb_Device_t *dev);
void vMBASCIITimerT1SExpired(mb_Device_t *dev);


mb_ErrorCode_t eMBMasterASCIIInit(void *dev, uint8_t ucPort,uint32_t ulBaudRate, mb_Parity_t eParity);
void vMBMasterASCIIStart(void *dev);
void vMBMasterASCIIStop(void *dev);
void vMBMasterASCIIClose(void *dev);
mb_ErrorCode_t eMBMasterASCIIReceive(void *dev, uint8_t *pucRcvAddress, uint8_t **pPdu,uint16_t *pusLength);
mb_ErrorCode_t eMBMasterASCIISend(void *pdev,const uint8_t *pAdu, uint16_t usLength);

void vMBMasterASCIIReceiveFSM(mb_MasterDevice_t *dev);
void vMBMasterASCIITransmitFSM(mb_MasterDevice_t *dev);
void vMBMasterASCIITimerT1SExpired(mb_MasterDevice_t *dev);


#ifdef __cplusplus
}
#endif

#endif

