
#include "port.h"
#include "mbutils.h"
#include "mbascii.h"


#if MB_ASCII_ENABLED > 0 && MB_SLAVE_ENABLED > 0

MbErrorCode_t MbsASCIIInit(MbsHandle_t dev, uint8_t port, uint32_t baudRate, MbParity_t parity) {
    (void) dev;
    MbErrorCode_t status = MB_ENOERR;

    ENTER_CRITICAL_SECTION();

    if (!MbPortSerialInit(port, baudRate, 7, parity) ||
        !MbPortTimersInit(port, MBS_ASCII_TIMEOUT_SEC * 20000UL)) {
        status = MB_EPORTERR;
    }

    EXIT_CRITICAL_SECTION();

    return status;
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

void MbsASCIIClose(MbsHandle_t dev) {}

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

        aduFrame->functionCode = pdev->AduBuf[MB_SER_ADU_PDU_OFFSET + MB_PDU_FUNCODE_OFF];
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

MbErrorCode_t MbsASCIISend(MbsHandle_t dev, uint8_t slaveID, const uint8_t *pPdu, uint16_t len) {
    uint8_t lrc;
    uint8_t *pAdu;
    uint8_t byte;
    MbsDev_t *pDev = (MbsDev_t *) dev;
    MbErrorCode_t status = MB_ENOERR;

    ENTER_CRITICAL_SECTION();
    /* Check if the receiver is still in idle state. If not we where too
     * slow with processing the received frame and the master sent another
     * frame on the network. We have to abort sending the frame.
     */
    if (pDev->sendRcvState == STATE_ASCII_RX_IDLE) {
        /* First byte before the Modbus-PDU is the slave address. */
        pAdu = (uint8_t *) pPdu - 1;
        pDev->sendAduBuffCount = 1;

        /* Now copy the Modbus-PDU into the Modbus-Serial-Line-PDU. */
        pAdu[MB_SER_ADU_ADDR_OFFSET] = slaveID;
        pDev->sendAduBuffCount += len;

        /* Calculate LRC checksum for Modbus-Serial-Line-PDU. */
        lrc = MbLRC((uint8_t *) pAdu, pDev->sendAduBuffCount);
        pDev->AduBuf[pDev->sendAduBuffCount++] = lrc;

        /* Activate the transmitter. */
        pDev->sendRcvState = STATE_ASCII_TX_START;

        /* start the first transmitter then into serial tc interrupt */
        byte = ':';
        MbPortSerialPutByte(pDev->port, (char) byte);
        pDev->sendAduBuffPos = 0;
        pDev->AsciiBytePos = BYTE_HIGH_NIBBLE;
        pDev->sendRcvState = STATE_ASCII_TX_DATA;

        MbPortSerialEnable(pDev->port, false, true);
    } else {
        status = MB_EIO;
    }
    EXIT_CRITICAL_SECTION();

    return status;
}

void MbsASCIIReceiveFSM(MbsHandle_t dev) {
    uint8_t byte;
    uint8_t result;
    MbsDev_t *pDev = (MbsDev_t *) dev;

    (void) MbPortSerialGetByte(pDev->port, (char *) &byte);
    switch (pDev->sendRcvState) {
        /* A new character is received. If the character is a ':' the input
         * buffer is cleared. A CR-character signals the end of the data
         * block. Other characters are part of the data block and their
         * ASCII value is converted back to a binary representation.
         */
        case STATE_ASCII_RX_RCV:
            /* Enable timer for character timeout. */
            MbPortTimersEnable(pDev->port);
            if (byte == ':') {
                /* Empty receive buffer. */
                pDev->AsciiBytePos = BYTE_HIGH_NIBBLE;
                pDev->rcvAduBuffPos = 0;
            } else if (byte == MB_ASCII_DEFAULT_CR) {
                pDev->sendRcvState = STATE_ASCII_RX_WAIT_EOF;
            } else {
                result = MbChar2Bin(byte);
                switch (pDev->AsciiBytePos) {
                    /* High nibble of the byte comes first. We check for
                     * a buffer overflow here. */
                    case BYTE_HIGH_NIBBLE:
                        if (pDev->rcvAduBuffPos < MB_ADU_SIZE_MAX) {
                            pDev->AduBuf[pDev->rcvAduBuffPos] = (uint8_t) (result << 4);
                            pDev->AsciiBytePos = BYTE_LOW_NIBBLE;
                            break;
                        } else {
                            /* not handled in Modbus specification but seems
                             * a resonable implementation. */
                            pDev->sendRcvState = STATE_ASCII_RX_IDLE;
                            /* Disable previously activated timer because of error state. */
                            MbPortTimersDisable(pDev->port);
                        }
                        break;

                    case BYTE_LOW_NIBBLE:
                        pDev->AduBuf[pDev->rcvAduBuffPos] |= result;
                        pDev->rcvAduBuffPos++;
                        pDev->AsciiBytePos = BYTE_HIGH_NIBBLE;
                        break;
                }
            }
            break;

        case STATE_ASCII_RX_WAIT_EOF:
            if (byte == MB_ASCII_DEFAULT_LF) {
                /* Disable character timeout timer because all characters are
                 * received. */
                MbPortTimersDisable(pDev->port);
                /* Receiver is again in idle state. */
                pDev->sendRcvState = STATE_ASCII_RX_IDLE;

                /* Notify the caller of MbsASCIIReceive that a new frame was received. */
                MbsSemGive(pDev);
            } else if (byte == ':') {
                /* Empty receive buffer and back to receive state. */
                pDev->AsciiBytePos = BYTE_HIGH_NIBBLE;
                pDev->rcvAduBuffPos = 0;
                pDev->sendRcvState = STATE_ASCII_RX_RCV;

                /* Enable timer for character timeout. */
                MbPortTimersEnable(pDev->port);
            } else {
                /* Frame is not okay. Delete entire frame. */
                pDev->sendRcvState = STATE_ASCII_RX_IDLE;
            }
            break;

        case STATE_ASCII_RX_IDLE:
            if (byte == ':') {
                /* Enable timer for character timeout. */
                MbPortTimersEnable(pDev->port);
                /* Reset the input buffers to store the frame. */
                pDev->rcvAduBuffPos = 0;
                pDev->AsciiBytePos = BYTE_HIGH_NIBBLE;
                pDev->sendRcvState = STATE_ASCII_RX_RCV;
            }
            break;
    }
}

void MbsASCIITransmitFSM(MbsHandle_t dev) {
    uint8_t byte;
    MbsDev_t *pDev = (MbsDev_t *) dev;

    switch (pDev->sendRcvState) {
        /* Start of transmission. The start of a frame is defined by sending
         * the character ':'. */
        case STATE_ASCII_TX_START:
            byte = ':';
            MbPortSerialPutByte(pDev->port, (char) byte);
            pDev->sendAduBuffPos = 0;
            pDev->AsciiBytePos = BYTE_HIGH_NIBBLE;
            pDev->sendRcvState = STATE_ASCII_TX_DATA;
            break;

            /* Send the data block. Each data byte is encoded as a character hex
             * stream with the high nibble sent first and the low nibble sent
             * last. If all data bytes are exhausted we send a '\r' character
             * to end the transmission. */
        case STATE_ASCII_TX_DATA:
            if (pDev->sendAduBuffCount > 0) {
                switch (pDev->AsciiBytePos) {
                    case BYTE_HIGH_NIBBLE:
                        byte = MbBin2Char((uint8_t) (pDev->AduBuf[pDev->sendAduBuffPos] >> 4));
                        MbPortSerialPutByte(pDev->port, (char) byte);
                        pDev->AsciiBytePos = BYTE_LOW_NIBBLE;
                        break;

                    case BYTE_LOW_NIBBLE:
                        byte = MbBin2Char((uint8_t) (pDev->AduBuf[pDev->sendAduBuffPos] & 0x0F));
                        MbPortSerialPutByte(pDev->port, (char) byte);
                        pDev->sendAduBuffPos++;
                        pDev->sendAduBuffCount--;
                        pDev->AsciiBytePos = BYTE_HIGH_NIBBLE;
                        break;
                }
            } else {
                MbPortSerialPutByte(pDev->port, (char) MB_ASCII_DEFAULT_CR);
                pDev->sendRcvState = STATE_ASCII_TX_END;
            }
            break;

            /* Finish the frame by sending a LF character. */
        case STATE_ASCII_TX_END:
            MbPortSerialPutByte(pDev->port, (char) MB_ASCII_DEFAULT_LF);
            /* We need another state to make sure that the CR character has
             * been sent. */
            pDev->sendRcvState = STATE_ASCII_TX_NOTIFY;
            break;

            /* Notify the task which called MbsASCIISend that the frame has
             * been sent. */
        case STATE_ASCII_TX_NOTIFY:
            /* Disable transmitter. This prevents another transmit buffer
             * empty interrupt. */
            MbPortSerialEnable(pDev->port, true, false);
            pDev->sendRcvState = STATE_ASCII_RX_IDLE;
            break;
    }
}

void MbsASCIITimerT1SExpired(MbsHandle_t dev) {
    MbsDev_t *pDev = (MbsDev_t *) dev;

    /* If we have a timeout we go back to the idle state and wait for
     * the next frame.
     */
    if ((pDev->sendRcvState == STATE_ASCII_RX_RCV) || (pDev->sendRcvState == STATE_ASCII_RX_WAIT_EOF)) {
        pDev->sendRcvState = STATE_ASCII_RX_IDLE;
    }
    MbPortTimersDisable(pDev->port);
}

#endif
