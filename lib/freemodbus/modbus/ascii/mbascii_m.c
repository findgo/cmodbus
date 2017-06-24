
#include "port.h"
#include "mbascii.h"


#if MB_MASTER_ENABLED > 0
mb_ErrorCode_t eMBMasterASCIIInit(void *dev, uint8_t ucPort,uint32_t ulBaudRate, mb_Parity_t eParity )
{
}
void vMBMasterASCIIStart(void *dev)
{
}
void vMBMasterASCIIStop(void *dev)
{
}
void vMBMasterASCIIClose(void *dev)
{
}
mb_ErrorCode_t eMBMasterASCIIReceive(void *dev, uint8_t *pucRcvAddress, uint8_t **pPdu,uint16_t *pusLength )
{
}
mb_ErrorCode_t eMBMasterASCIISend(void *pdev,const uint8_t *pAdu, uint16_t usLength)
{
}

void vMBMasterASCIIReceiveFSM(mb_MasterDevice_t *dev)
{
}
void vMBMasterASCIITransmitFSM(mb_MasterDevice_t *dev)
{
}
void vMBMasterASCIITimerT1SExpired(mb_MasterDevice_t *dev)
{
}


#endif

