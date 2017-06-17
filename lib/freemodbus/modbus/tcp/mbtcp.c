

#include "mbtcp.h"

#if MB_TCP_ENABLED > 0

#define MB_TCP_PROTOCOL_ID  0   /* 0 = Modbus Protocol */

/* ----------------------- Start implementation -----------------------------*/
eMBErrorCode eMBTCPDoInit(uint16_t ucTCPPort)
{
    if( xMBTCPPortInit( ucTCPPort ) == false )
        return MB_EPORTERR;
    
    return MB_ENOERR;
}

void eMBTCPStart(  mb_device_t *dev)
{

}

void eMBTCPStop(  mb_device_t *dev)
{
    /* Make sure that no more clients are connected. */
    vMBTCPPortDisable( );
}

eMBErrorCode eMBTCPReceive(mb_device_t *dev, uint8_t *pucRcvAddress, uint8_t **pPdu, uint16_t *pusLength )
{
    eMBErrorCode    eStatus = MB_EIO;
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

eMBErrorCode eMBTCPSend(mb_device_t *dev, uint8_t _unused, const uint8_t *pPdu, uint16_t usLength )
{
    uint8_t          *pAdu = ( uint8_t * ) pPdu - MB_TCP_ADU_PDU_OFFSET;
    uint16_t          usTCPAduLength = usLength + MB_TCP_ADU_PDU_OFFSET;

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
