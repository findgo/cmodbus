
#include "port.h"
#include "mbutils.h"
#include "mbascii.h"


#if MB_ASCII_ENABLED > 0 && MB_SLAVE_ENABLED > 0

MbErrorCode_t MbsASCIIInit(MbsHandle_t dev, uint8_t ucPort, uint32_t ulBaudRate, MbParity_t eParity) {
    MbErrorCode_t eStatus = MB_ENOERR;
    (void) dev;

    ENTER_CRITICAL_SECTION();

    if ((MbPortSerialInit(ucPort, ulBaudRate, 7, eParity) != true) \
 || (MbPortTimersInit(ucPort, MBS_ASCII_TIMEOUT_SEC * 20000UL) != true)) {
        eStatus = MB_EPORTERR;
    }

    EXIT_CRITICAL_SECTION();

    return eStatus;
}

void MbsASCIIStart(MbsHandle_t dev) {
    ENTER_CRITICAL_SECTION();

    ((MbsDev_t *) dev)->sendRcvState = STATE_ASCII_RX_IDLE;
    MbPortSerialEnable(((MbsDev_t *) dev)->port, true, false);

    EXIT_CRITICAL_SECTION();
}

void MbsASCIIStop(MbsHandle_t dev) {
    ENTER_CRITICAL_SECTION();

    MbPortSerialEnable(((MbsDev_t *) dev)->port, false, false);
    MbPortTimersDisable(((MbsDev_t *) dev)->port);

    EXIT_CRITICAL_SECTION();
}

void MbsASCIIClose(MbsHandle_t dev) {

}

MbErrorCode_t MbsASCIIReceiveParse(MbsHandle_t dev, MbsAduFrame_t *aduFrame) {
    MbErrorCode_t eStatus = MB_ENOERR;
    MbsDev_t *pdev = (MbsDev_t *) dev;

    ENTER_CRITICAL_SECTION();

    /* Length and LRC check */
    if ((pdev->rcvAduBuffPos >= MB_ADU_ASCII_SIZE_MIN)
        && (MbLRC((uint8_t *) pdev->AduBuf, pdev->rcvAduBuffPos) == 0)) {
        /* Save the address field. All frames are passed to the upper layed
         * and the decision if a frame is used is done there.
         */
        aduFrame->hdr.introute.slaveID = pdev->AduBuf[MB_SER_ADU_ADDR_OFFSET];

        aduFrame->FunctionCode = pdev->AduBuf[MB_SER_ADU_PDU_OFFSET + MB_PDU_FUNCODE_OFF];
        /* Total length of Modbus-PDU is Modbus-Serial-Line-PDU minus
         * size of address field and CRC checksum.
         */
        aduFrame->pduFrameLength = (uint16_t) (pdev->rcvAduBuffPos - MB_SER_ADU_SIZE_ADDR - MB_SER_ADU_SIZE_LRC);

        /* Return the start of the Modbus PDU to the caller. */
        aduFrame->pPduFrame = (uint8_t *) &pdev->AduBuf[MB_SER_ADU_PDU_OFFSET];
    } else {
        eStatus = MB_EIO;
    }

    EXIT_CRITICAL_SECTION();

    return eStatus;
}

MbErrorCode_t MbsASCIISend(MbsHandle_t dev, uint8_t ucSlaveAddress, const uint8_t *pPdu, uint16_t usLength) {
    MbErrorCode_t eStatus = MB_ENOERR;
    uint8_t usLRC;
    uint8_t *pAdu;
    uint8_t ucByte;
    MbsDev_t *pdev = (MbsDev_t *) dev;

    ENTER_CRITICAL_SECTION();
    /* Check if the receiver is still in idle state. If not we where too
     * slow with processing the received frame and the master sent another
     * frame on the network. We have to abort sending the frame.
     */
    if (pdev->sendRcvState == STATE_ASCII_RX_IDLE) {

        /* First byte before the Modbus-PDU is the slave address. */
        pAdu = (uint8_t *) pPdu - 1;
        pdev->sendAduBuffCount = 1;

        /* Now copy the Modbus-PDU into the Modbus-Serial-Line-PDU. */
        pAdu[MB_SER_ADU_ADDR_OFFSET] = ucSlaveAddress;
        pdev->sendAduBuffCount += usLength;

        /* Calculate LRC checksum for Modbus-Serial-Line-PDU. */
        usLRC = MbLRC((uint8_t *) pAdu, pdev->sendAduBuffCount);
        pdev->AduBuf[pdev->sendAduBuffCount++] = usLRC;

        /* Activate the transmitter. */
        pdev->sendRcvState = STATE_ASCII_TX_START;

        /* start the first transmitter then into serial tc interrupt */
        ucByte = ':';
        MbPortSerialPutByte(pdev->port, (char) ucByte);
        pdev->sendAduBuffPos = 0;
        pdev->AsciiBytePos = BYTE_HIGH_NIBBLE;
        pdev->sendRcvState = STATE_ASCII_TX_DATA;

        MbPortSerialEnable(pdev->port, false, true);
    } else {
        eStatus = MB_EIO;
    }
    EXIT_CRITICAL_SECTION();

    return eStatus;
}

void MbsASCIIReceiveFSM(MbsHandle_t dev) {
    uint8_t ucByte;
    uint8_t ucResult;
    MbsDev_t *pdev = (MbsDev_t *) dev;

    (void) MbPortSerialGetByte(pdev->port, (char *) &ucByte);
    switch (pdev->sendRcvState) {
        /* A new character is received. If the character is a ':' the input
         * buffer is cleared. A CR-character signals the end of the data
         * block. Other characters are part of the data block and their
         * ASCII value is converted back to a binary representation.
         */
        case STATE_ASCII_RX_RCV:
            /* Enable timer for character timeout. */
            MbPortTimersEnable(pdev->port);
            if (ucByte == ':') {
                /* Empty receive buffer. */
                pdev->AsciiBytePos = BYTE_HIGH_NIBBLE;
                pdev->rcvAduBuffPos = 0;
            } else if (ucByte == MB_ASCII_DEFAULT_CR) {
                pdev->sendRcvState = STATE_ASCII_RX_WAIT_EOF;
            } else {
                ucResult = MbChar2Bin(ucByte);
                switch (pdev->AsciiBytePos) {
                    /* High nibble of the byte comes first. We check for
                     * a buffer overflow here. */
                    case BYTE_HIGH_NIBBLE:
                        if (pdev->rcvAduBuffPos < MB_ADU_SIZE_MAX) {
                            pdev->AduBuf[pdev->rcvAduBuffPos] = (uint8_t) (ucResult << 4);
                            pdev->AsciiBytePos = BYTE_LOW_NIBBLE;
                            break;
                        } else {
                            /* not handled in Modbus specification but seems
                             * a resonable implementation. */
                            pdev->sendRcvState = STATE_ASCII_RX_IDLE;
                            /* Disable previously activated timer because of error state. */
                            MbPortTimersDisable(pdev->port);
                        }
                        break;

                    case BYTE_LOW_NIBBLE:
                        pdev->AduBuf[pdev->rcvAduBuffPos] |= ucResult;
                        pdev->rcvAduBuffPos++;
                        pdev->AsciiBytePos = BYTE_HIGH_NIBBLE;
                        break;
                }
            }
            break;

        case STATE_ASCII_RX_WAIT_EOF:
            if (ucByte == MB_ASCII_DEFAULT_LF) {
                /* Disable character timeout timer because all characters are
                 * received. */
                MbPortTimersDisable(pdev->port);
                /* Receiver is again in idle state. */
                pdev->sendRcvState = STATE_ASCII_RX_IDLE;

                /* Notify the caller of MbsASCIIReceive that a new frame was received. */
                MbsSemGive(pdev);
            } else if (ucByte == ':') {
                /* Empty receive buffer and back to receive state. */
                pdev->AsciiBytePos = BYTE_HIGH_NIBBLE;
                pdev->rcvAduBuffPos = 0;
                pdev->sendRcvState = STATE_ASCII_RX_RCV;

                /* Enable timer for character timeout. */
                MbPortTimersEnable(pdev->port);
            } else {
                /* Frame is not okay. Delete entire frame. */
                pdev->sendRcvState = STATE_ASCII_RX_IDLE;
            }
            break;

        case STATE_ASCII_RX_IDLE:
            if (ucByte == ':') {
                /* Enable timer for character timeout. */
                MbPortTimersEnable(pdev->port);
                /* Reset the input buffers to store the frame. */
                pdev->rcvAduBuffPos = 0;
                pdev->AsciiBytePos = BYTE_HIGH_NIBBLE;
                pdev->sendRcvState = STATE_ASCII_RX_RCV;
            }
            break;
    }
}

void MbsASCIITransmitFSM(MbsHandle_t dev) {
    uint8_t ucByte;
    MbsDev_t *pdev = (MbsDev_t *) dev;

    switch (pdev->sendRcvState) {
        /* Start of transmission. The start of a frame is defined by sending
         * the character ':'. */
        case STATE_ASCII_TX_START:
            ucByte = ':';
            MbPortSerialPutByte(pdev->port, (char) ucByte);
            pdev->sendAduBuffPos = 0;
            pdev->AsciiBytePos = BYTE_HIGH_NIBBLE;
            pdev->sendRcvState = STATE_ASCII_TX_DATA;
            break;

            /* Send the data block. Each data byte is encoded as a character hex
             * stream with the high nibble sent first and the low nibble sent
             * last. If all data bytes are exhausted we send a '\r' character
             * to end the transmission. */
        case STATE_ASCII_TX_DATA:
            if (pdev->sendAduBuffCount > 0) {
                switch (pdev->AsciiBytePos) {
                    case BYTE_HIGH_NIBBLE:
                        ucByte = MbBin2Char((uint8_t) (pdev->AduBuf[pdev->sendAduBuffPos] >> 4));
                        MbPortSerialPutByte(pdev->port, (char) ucByte);
                        pdev->AsciiBytePos = BYTE_LOW_NIBBLE;
                        break;

                    case BYTE_LOW_NIBBLE:
                        ucByte = MbBin2Char((uint8_t) (pdev->AduBuf[pdev->sendAduBuffPos] & 0x0F));
                        MbPortSerialPutByte(pdev->port, (char) ucByte);
                        pdev->sendAduBuffPos++;
                        pdev->sendAduBuffCount--;
                        pdev->AsciiBytePos = BYTE_HIGH_NIBBLE;
                        break;
                }
            } else {
                MbPortSerialPutByte(pdev->port, (char) MB_ASCII_DEFAULT_CR);
                pdev->sendRcvState = STATE_ASCII_TX_END;
            }
            break;

            /* Finish the frame by sending a LF character. */
        case STATE_ASCII_TX_END:
            MbPortSerialPutByte(pdev->port, (char) MB_ASCII_DEFAULT_LF);
            /* We need another state to make sure that the CR character has
             * been sent. */
            pdev->sendRcvState = STATE_ASCII_TX_NOTIFY;
            break;

            /* Notify the task which called MbsASCIISend that the frame has
             * been sent. */
        case STATE_ASCII_TX_NOTIFY:
            /* Disable transmitter. This prevents another transmit buffer
             * empty interrupt. */
            MbPortSerialEnable(pdev->port, true, false);
            pdev->sendRcvState = STATE_ASCII_RX_IDLE;
            break;
    }
}

void MbsASCIITimerT1SExpired(MbsHandle_t dev) {
    MbsDev_t *pdev = (MbsDev_t *) dev;

    /* If we have a timeout we go back to the idle state and wait for
     * the next frame.
     */
    if ((pdev->sendRcvState == STATE_ASCII_RX_RCV) || (pdev->sendRcvState == STATE_ASCII_RX_WAIT_EOF)) {
        pdev->sendRcvState = STATE_ASCII_RX_IDLE;
    }
    MbPortTimersDisable(pdev->port);
}

#endif
