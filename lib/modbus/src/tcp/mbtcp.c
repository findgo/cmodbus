

#include "mbtcp.h"

#if MB_TCP_ENABLED > 0

/* ----------------------- Start implementation -----------------------------*/
#if MB_SLAVE_ENABLED > 0

MbErrCode_t MbsTCPInit(uint16_t port) {
    if (MbsTCPPortInit(port) == false)
        return MB_EPORTERR;

    return MB_ESUCCESS;
}

void MbsTCPStart(void *dev) {

}

void MbsTCPStop(void *dev) {
    /* Make sure that no more clients are connected. */
    MbTCPPortDisable();
}

MbErrCode_t MbsTCPReceive(void *dev, uint8_t *pRcvAddress, uint8_t **pPdu, uint16_t *pLen) {
    MbErrCode_t eStatus = MB_EIO;
    uint8_t *pucMBTCPFrame;
    uint16_t usLength;
    uint16_t usPID;

    if (MbTCPPortGetRequest(&pucMBTCPFrame, &usLength) != false) {

        usPID = pucMBTCPFrame[MB_TCP_ADU_PID_OFFSET] << 8U;
        usPID |= pucMBTCPFrame[MB_TCP_ADU_PID_OFFSET + 1];

        if (usPID == MB_TCP_PROTOCOL_ID) {

            *pPdu = &pucMBTCPFrame[MB_TCP_ADU_PDU_OFFSET];
            *pLen = usLength - MB_TCP_ADU_PDU_OFFSET;
            eStatus = MB_ESUCCESS;

            /* Modbus TCP does not use any addresses. Fake the source address such
             * that the processing part deals with this frame.
             */
            *pRcvAddress = MB_TCP_PSEUDO_ADDRESS;
        }
    } else {
        eStatus = MB_EIO;
    }

    return eStatus;
}

MbErrCode_t MbsTCPSend(void *dev, uint8_t _unused, const uint8_t *pPdu, uint16_t len) {
    uint8_t *pAdu = (uint8_t *) pPdu - MB_TCP_ADU_PDU_OFFSET;
    uint16_t usTCPAduLength = len + MB_TCP_ADU_PDU_OFFSET;

    /* The MBAP header is already initialized because the caller calls this
     * function with the buffer returned by the previous call. Therefore we 
     * only have to update the length in the header. Note that the length 
     * header includes the size of the Modbus PDU and the UID Byte. Therefore 
     * the length is len plus one.
     */
    pAdu[MB_TCP_ADU_LEN_OFFSET] = (len + 1) >> 8U;
    pAdu[MB_TCP_ADU_LEN_OFFSET + 1] = (len + 1) & 0xFF;
    if (MbTCPPortSendResponse(pAdu, usTCPAduLength) == false)
        return MB_EIO;

    return MB_ESUCCESS;
}

#endif

#if MB_MASTER_ENABLED > 0
MbErrCode_t MbmTCPInit(uint16_t ucTCPPort)
{
}
void MbmTCPStart(void *dev)
{
}
void MbmTCPStop(void *dev)
{
}
void MbmTCPClose(void *dev)
{
}
MbErrCode_t MbmTCPReceive(void *dev, uint8_t *pucRcvAddress, uint8_t **pPdu,uint16_t *pusLength)
{
}
MbErrCode_t MbmTCPSend(void *pdev,const uint8_t *pAdu, uint16_t usLength)
{
}
#endif

#endif

