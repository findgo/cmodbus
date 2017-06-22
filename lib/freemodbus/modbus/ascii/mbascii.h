
#ifndef __MB_ASCII_H
#define __MB_ASCII_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mbconfig.h"
#include "mbproto.h"
#include "mbframe.h"
#include "mbcpu.h"

#include "modbus.h"
#include "mbutils.h"
#include "mbevent.h"

#include "port.h"

/* ----------------------- Defines ------------------------------------------*/
#define MB_ASCII_DEFAULT_CR     '\r'    /*!< Default CR character for Modbus ASCII. */
#define MB_ASCII_DEFAULT_LF     '\n'    /*!< Default LF character for Modbus ASCII. */


mb_ErrorCode_t eMBASCIIInit(void *dev, uint8_t ucPort,uint32_t ulBaudRate, mb_Parity_t eParity);
void vMBASCIIStart(void *dev);
void vMBASCIIStop(void *dev);
void vMBASCIIClose(void *dev);
mb_ErrorCode_t eMBASCIIReceive(void *dev, uint8_t *pucRcvAddress, uint8_t **pPdu,uint16_t *pusLength);
mb_ErrorCode_t eMBASCIISend(void *dev, uint8_t ucSlaveAddress, const uint8_t *pPdu, uint16_t usLength);

bool xMBASCIIReceiveFSM(mb_Device_t *dev);
bool xMBASCIITransmitFSM(mb_Device_t *dev);
bool xMBASCIITimerT1SExpired(mb_Device_t *dev);



mb_ErrorCode_t eMBMasterASCIIInit(void *dev, uint8_t ucPort,uint32_t ulBaudRate, mb_Parity_t eParity);
void vMBMasterASCIIStart(void *dev);
void vMBMasterASCIIStop(void *dev);
void vMBMasterASCIIClose(void *dev);
mb_ErrorCode_t eMBMasterASCIIReceive(void *dev, uint8_t *pucRcvAddress, uint8_t **pPdu,uint16_t *pusLength);
mb_ErrorCode_t eMBMasterASCIISend(void *pdev,const uint8_t *pAdu, uint16_t usLength);

bool xMBMasterASCIIReceiveFSM(mb_MasterDevice_t *dev);
bool xMBMasterASCIITransmitFSM(mb_MasterDevice_t *dev);
bool xMBMasterASCIITimerT1SExpired(mb_MasterDevice_t *dev);


#ifdef __cplusplus
}
#endif

#endif

