

#include "mbtcp.h"

#if MB_TCP_ENABLED > 0

/* ----------------------- Start implementation -----------------------------*/
#if MB_SLAVE_ENABLE > 0

mb_ErrorCode_t eMBTCPInit(uint16_t ucTCPPort)
{
    if( xMBTCPPortInit( ucTCPPort ) == false )
        return MB_EPORTERR;
    
    return MB_ENOERR;
}

void vMBTCPStart(  void *dev)
{

}

void vMBTCPStop(  void *dev)
{
    /* Make sure that no more clients are connected. */
    vMBTCPPortDisable( );
}

mb_ErrorCode_t eMBTCPReceive(void *dev, uint8_t *pucRcvAddress, uint8_t **pPdu, uint16_t *pusLength )
{
    mb_ErrorCode_t    eStatus = MB_EIO;
    uint8_t          *pucMBTCPFrame;
    uint16_t          usLength;
    uint16_t          usPID;

    if( xMBTCPPortGetRequest( &pucMBTCPFrame, &usLength ) != false ) {
        
        usPID = pucMBTCPFrame[MB_TCP_ADU_PID_OFFSET] << 8U;
        usPID |= pucMBTCPFrame[MB_TCP_ADU_PID_OFFSET + 1];

        if( usPID == MB_TCP_PROTOCOL_ID ){
            
            *pPdu = &pucMBTCPFrame[MB_TCP_ADU_PDU_OFFSET];
            *pusLength = usLength - MB_TCP_ADU_PDU_OFFSET;
            eStatus = MB_ENOERR;

            /* Modbus TCP does not use any addresses. Fake the source address such
             * that the processing part deals with this frame.
             */
            *pucRcvAddress = MB_TCP_PSEUDO_ADDRESS;
        }
    }
    else{
        eStatus = MB_EIO;
    }
    
    return eStatus;
}

mb_ErrorCode_t eMBTCPSend(void *dev, uint8_t _unused, const uint8_t *pPdu, uint16_t usLength )
{
    uint8_t *pAdu = ( uint8_t * ) pPdu - MB_TCP_ADU_PDU_OFFSET;
    uint16_t usTCPAduLength = usLength + MB_TCP_ADU_PDU_OFFSET;

    /* The MBAP header is already initialized because the caller calls this
     * function with the buffer returned by the previous call. Therefore we 
     * only have to update the length in the header. Note that the length 
     * header includes the size of the Modbus PDU and the UID Byte. Therefore 
     * the length is usLength plus one.
     */
    pAdu[MB_TCP_ADU_LEN_OFFSET] = ( usLength + 1 ) >> 8U;
    pAdu[MB_TCP_ADU_LEN_OFFSET + 1] = ( usLength + 1 ) & 0xFF;
    if( xMBTCPPortSendResponse( pAdu, usTCPAduLength ) == false )
        return MB_EIO;
    
    return MB_ENOERR;
}

#endif

#if MB_MASTER_ENABLE > 0
mb_ErrorCode_t eMBMasterTCPInit(uint16_t ucTCPPort)
{
}
void vMBMasterTCPStart(void *dev)
{
}
void vMBMasterTCPStop(void *dev)
{
}
void vMBMasterTCPClose(void *dev)
{
}
mb_ErrorCode_t eMBMasterTCPReceive(void *dev, uint8_t *pucRcvAddress, uint8_t **pPdu,uint16_t *pusLength)
{
}
mb_ErrorCode_t eMBMasterTCPSend(void *pdev,const uint8_t *pAdu, uint16_t usLength)
{
}


#endif

#endif
