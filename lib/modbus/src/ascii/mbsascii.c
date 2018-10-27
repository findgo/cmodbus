
#include "port.h"
#include "mbutils.h"
#include "mbascii.h"


#if MB_ASCII_ENABLED > 0 && MB_SLAVE_ENABLED > 0

MbErrorCode_t MbsASCIIInit(void *dev, uint8_t ucPort, uint32_t ulBaudRate, MbParity_t eParity)
{
    MbErrorCode_t eStatus = MB_ENOERR;
    (void)dev;
    
    ENTER_CRITICAL_SECTION();

    if( (MbPortSerialInit( ucPort, ulBaudRate, 7, eParity) != TRUE) \
            || (MbPortTimersInit(ucPort, MBS_ASCII_TIMEOUT_SEC * 20000UL) != TRUE) ){
        eStatus = MB_EPORTERR;
    }

    EXIT_CRITICAL_SECTION();

    return eStatus;
}

void MbsASCIIStart(void *dev)
{
    ENTER_CRITICAL_SECTION();

    ((MbsDev_t *)dev)->sndrcvState = STATE_ASCII_RX_IDLE;
    MbPortSerialEnable(((MbsDev_t *)dev)->port, TRUE, FALSE);
    
    EXIT_CRITICAL_SECTION();
}

void MbsASCIIStop(void *dev)
{
    ENTER_CRITICAL_SECTION();
    
    MbPortSerialEnable(((MbsDev_t *)dev)->port, FALSE, FALSE);
    MbPortTimersDisable(((MbsDev_t *)dev)->port);
    
    EXIT_CRITICAL_SECTION();
}

void MbsASCIIClose(void *dev)
{

}

MbErrorCode_t MbsASCIIReceive(void *dev,uint8_t *pucRcvAddress, uint8_t **pPdu, uint16_t *pusLength)
{
    MbErrorCode_t eStatus = MB_ENOERR;
    MbsDev_t *pdev = (MbsDev_t *)dev;

    ENTER_CRITICAL_SECTION();

    /* Length and LRC check */
    if((pdev->rcvAduBufPos >= MB_ADU_ASCII_SIZE_MIN)
        && (MbLRC((uint8_t *)pdev->AduBuf, pdev->rcvAduBufPos) == 0)){
        /* Save the address field. All frames are passed to the upper layed
         * and the decision if a frame is used is done there.
         */
        *pucRcvAddress = pdev->AduBuf[MB_SER_ADU_ADDR_OFFSET];

        /* Total length of Modbus-PDU is Modbus-Serial-Line-PDU minus
         * size of address field and CRC checksum.
         */
        *pusLength = (uint16_t)(pdev->rcvAduBufPos - MB_SER_ADU_SIZE_ADDR - MB_SER_ADU_SIZE_LRC);

        /* Return the start of the Modbus PDU to the caller. */
        *pPdu = (uint8_t *)&pdev->AduBuf[MB_SER_ADU_PDU_OFFSET];
    }
    else{
        eStatus = MB_EIO;
    }
    
    EXIT_CRITICAL_SECTION();
    
    return eStatus;
}

MbErrorCode_t MbsASCIISend(void *dev, uint8_t ucSlaveAddress, const uint8_t *pPdu, uint16_t usLength)
{
    MbErrorCode_t eStatus = MB_ENOERR;
    uint8_t usLRC;
    uint8_t *pAdu;
    uint8_t ucByte;
    MbsDev_t *pdev = (MbsDev_t *)dev;
    
    ENTER_CRITICAL_SECTION(  );
    /* Check if the receiver is still in idle state. If not we where too
     * slow with processing the received frame and the master sent another
     * frame on the network. We have to abort sending the frame.
     */
    if(pdev->sndrcvState == STATE_ASCII_RX_IDLE){
        
        /* First byte before the Modbus-PDU is the slave address. */
        pAdu = (uint8_t *)pPdu - 1;
        pdev->sndAduBufCount = 1;

        /* Now copy the Modbus-PDU into the Modbus-Serial-Line-PDU. */
        pAdu[MB_SER_ADU_ADDR_OFFSET] = ucSlaveAddress;
        pdev->sndAduBufCount += usLength;

        /* Calculate LRC checksum for Modbus-Serial-Line-PDU. */
        usLRC = MbLRC( (uint8_t *)pAdu, pdev->sndAduBufCount );
        pdev->AduBuf[pdev->sndAduBufCount++] = usLRC;

        /* Activate the transmitter. */
        pdev->sndrcvState = STATE_ASCII_TX_START;

        /* start the first transmitter then into serial tc interrupt */
        ucByte = ':';
        MbPortSerialPutByte(pdev->port,(char)ucByte);
        pdev->sndAduBufPos = 0;
        pdev->AsciiBytePos = BYTE_HIGH_NIBBLE;
        pdev->sndrcvState = STATE_ASCII_TX_DATA;
      
        MbPortSerialEnable(pdev->port, FALSE, TRUE );
    }
    else{
        eStatus = MB_EIO;
    }
    EXIT_CRITICAL_SECTION();
    
    return eStatus;
}

void MbsASCIIReceiveFSM(  MbsDev_t *dev)
{
    uint8_t ucByte;
    uint8_t ucResult;

    (void)MbPortSerialGetByte(dev->port, (char *)&ucByte );
    switch (dev->sndrcvState){
        /* A new character is received. If the character is a ':' the input
         * buffer is cleared. A CR-character signals the end of the data
         * block. Other characters are part of the data block and their
         * ASCII value is converted back to a binary representation.
         */
    case STATE_ASCII_RX_RCV:
        /* Enable timer for character timeout. */
        MbPortTimersEnable(dev->port);
        if( ucByte == ':' ){
            /* Empty receive buffer. */
            dev->AsciiBytePos = BYTE_HIGH_NIBBLE;
            dev->rcvAduBufPos = 0;
        }
        else if( ucByte == MB_ASCII_DEFAULT_CR ){
            dev->sndrcvState = STATE_ASCII_RX_WAIT_EOF;
        }
        else{
            ucResult = MbChar2Bin( ucByte );
            switch (dev->AsciiBytePos){
                /* High nibble of the byte comes first. We check for
                 * a buffer overflow here. */
            case BYTE_HIGH_NIBBLE:
                if( dev->rcvAduBufPos < MB_ADU_SIZE_MAX ){
                    dev->AduBuf[dev->rcvAduBufPos] = (uint8_t)(ucResult << 4);
                    dev->AsciiBytePos = BYTE_LOW_NIBBLE;
                    break;
                }
                else{
                    /* not handled in Modbus specification but seems
                     * a resonable implementation. */
                    dev->sndrcvState = STATE_ASCII_RX_IDLE;
                    /* Disable previously activated timer because of error state. */
                    MbPortTimersDisable(dev->port);
                }
                break;

            case BYTE_LOW_NIBBLE:
                dev->AduBuf[dev->rcvAduBufPos] |= ucResult;
                dev->rcvAduBufPos++;
                dev->AsciiBytePos = BYTE_HIGH_NIBBLE;
                break;
            }
        }
        break;

    case STATE_ASCII_RX_WAIT_EOF:
        if( ucByte == MB_ASCII_DEFAULT_LF ){
            /* Disable character timeout timer because all characters are
             * received. */
            MbPortTimersDisable(dev->port);
            /* Receiver is again in idle state. */
            dev->sndrcvState = STATE_ASCII_RX_IDLE;

            /* Notify the caller of MbsASCIIReceive that a new frame was received. */
            MbsSemGive(dev);
        }
        else if( ucByte == ':' ){
            /* Empty receive buffer and back to receive state. */
            dev->AsciiBytePos = BYTE_HIGH_NIBBLE;
            dev->rcvAduBufPos = 0;
            dev->sndrcvState = STATE_ASCII_RX_RCV;

            /* Enable timer for character timeout. */
            MbPortTimersEnable(dev->port);
        }
        else{
            /* Frame is not okay. Delete entire frame. */
            dev->sndrcvState = STATE_ASCII_RX_IDLE;
        }
        break;

    case STATE_ASCII_RX_IDLE:
        if( ucByte == ':' ){
            /* Enable timer for character timeout. */
            MbPortTimersEnable(dev->port);
            /* Reset the input buffers to store the frame. */
            dev->rcvAduBufPos = 0;
            dev->AsciiBytePos = BYTE_HIGH_NIBBLE;
            dev->sndrcvState = STATE_ASCII_RX_RCV;
        }
        break;
    }
}

void MbsASCIITransmitFSM(  MbsDev_t *dev)
{
    uint8_t ucByte;
    
    switch(dev->sndrcvState){
        /* Start of transmission. The start of a frame is defined by sending
         * the character ':'. */
    case STATE_ASCII_TX_START:
        ucByte = ':';
        MbPortSerialPutByte(dev->port, (char)ucByte );
        dev->sndAduBufPos = 0;
        dev->AsciiBytePos = BYTE_HIGH_NIBBLE;
        dev->sndrcvState = STATE_ASCII_TX_DATA;
        break;

        /* Send the data block. Each data byte is encoded as a character hex
         * stream with the high nibble sent first and the low nibble sent
         * last. If all data bytes are exhausted we send a '\r' character
         * to end the transmission. */
    case STATE_ASCII_TX_DATA:
        if( dev->sndAduBufCount > 0 ){
            switch(dev->AsciiBytePos){
            case BYTE_HIGH_NIBBLE:
                ucByte = MbBin2Char((uint8_t)(dev->AduBuf[dev->sndAduBufPos] >> 4));
                MbPortSerialPutByte(dev->port, (char)ucByte);
                dev->AsciiBytePos = BYTE_LOW_NIBBLE;
                break;

            case BYTE_LOW_NIBBLE:
                ucByte = MbBin2Char((uint8_t)(dev->AduBuf[dev->sndAduBufPos] & 0x0F));
                MbPortSerialPutByte(dev->port, (char)ucByte);
                dev->sndAduBufPos++;
                dev->sndAduBufCount--;                
                dev->AsciiBytePos = BYTE_HIGH_NIBBLE;
                break;
            }
        }
        else{
            MbPortSerialPutByte(dev->port, (char)MB_ASCII_DEFAULT_CR);
            dev->sndrcvState = STATE_ASCII_TX_END;
        }
        break;

        /* Finish the frame by sending a LF character. */
    case STATE_ASCII_TX_END:
        MbPortSerialPutByte(dev->port, (char)MB_ASCII_DEFAULT_LF);
        /* We need another state to make sure that the CR character has
         * been sent. */
        dev->sndrcvState = STATE_ASCII_TX_NOTIFY;
        break;

        /* Notify the task which called MbsASCIISend that the frame has
         * been sent. */
    case STATE_ASCII_TX_NOTIFY:
        /* Disable transmitter. This prevents another transmit buffer
         * empty interrupt. */
        MbPortSerialEnable(dev->port, TRUE, FALSE);
        dev->sndrcvState = STATE_ASCII_RX_IDLE;
        break;
    }
}

void MbsASCIITimerT1SExpired(  MbsDev_t *dev)
{
    /* If we have a timeout we go back to the idle state and wait for
     * the next frame.
     */
    if((dev->sndrcvState == STATE_ASCII_RX_RCV) || (dev->sndrcvState == STATE_ASCII_RX_WAIT_EOF)){
        dev->sndrcvState = STATE_ASCII_RX_IDLE;
    }
    MbPortTimersDisable(dev->port);
}
#endif
