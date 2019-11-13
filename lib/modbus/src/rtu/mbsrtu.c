
#include "port.h"
#include "mbrtu.h"
#include "mbutils.h"

#if MB_RTU_ENABLED > 0 && MB_SLAVE_ENABLED > 0

/**
 * @brief modbus从机初始化
 *
 * @param dev: 句柄
 * @param port: 端口
 * @param baudRate: 波特率
 * @param parity: 校验位
 * @return ::MbErrorCode_t
 */
MbErrorCode_t MbsRTUInit(MbsHandle_t dev, uint8_t port, uint32_t baudRate, MbParity_t parity) {
    (void) dev;
    MbErrorCode_t status = MB_ENOERR;
    uint32_t timerT35_50us;

    ENTER_CRITICAL_SECTION();
    /* modbus RTU uses 8 DataBits. */
    if (MbPortSerialInit(port, baudRate, 8, parity)) {
        /* If baudRate > 19200 then we should use the fixed timer values
        * t35 = 1750us. Otherwise t35 must be 3.5 times the character time.
        */
        if (baudRate > 19200) {
            timerT35_50us = 35;       /* 1800us. */
        } else {
            /* The timer reload value for a character is given by:
             *
             * ChTimeValue = Ticks_per_1s / ( baudRate / 11 )
             *             = 11 * Ticks_per_1s / baudRate
             *             = 220000 / baudRate
             * The reload for t3.5 is 1.5 times this value and similar
             * for t3.5.
             */
            timerT35_50us = (7UL * 220000UL) / (2UL * baudRate);
        }

        if (!MbPortTimersInit(port, (uint16_t) timerT35_50us)) {
            status = MB_EPORTERR;
        }
    } else {
        status = MB_EPORTERR;
    }
    EXIT_CRITICAL_SECTION();

    return status;
}

void MbsRTUStart(MbsHandle_t dev) {
    ENTER_CRITICAL_SECTION();

    ((MbsDev_t *) dev)->sendRcvState = STATE_RTU_RX_IDLE;
    MbPortSerialEnable(((MbsDev_t *) dev)->port, true, false);
    MbPortTimersDisable(((MbsDev_t *) dev)->port);

    EXIT_CRITICAL_SECTION();
}

void MbsRTUStop(MbsHandle_t dev) {
    ENTER_CRITICAL_SECTION();
    MbPortSerialEnable(((MbsDev_t *) dev)->port, false, false);
    MbPortTimersDisable(((MbsDev_t *) dev)->port);
    EXIT_CRITICAL_SECTION();
}

void MbsRTUClose(MbsHandle_t dev) {}

MbErrorCode_t MbsRTUReceiveParse(MbsHandle_t dev, MbsAduFrame_t *aduFrame) {
    MbErrorCode_t status = MB_ENOERR;
    MbsDev_t *pDev = (MbsDev_t *) dev;

    ENTER_CRITICAL_SECTION();
    /* Length and CRC check */
    if ((pDev->rcvAduBuffPos >= MB_ADU_RTU_SIZE_MIN)
        && (MbCRC16((uint8_t *) pDev->AduBuf, pDev->rcvAduBuffPos) == 0)) {

        /* Save the address field. All frames are passed to the upper layer
         * and the decision if a frame is used is done there.
         */
        aduFrame->hdr.inRoute.slaveID = pDev->AduBuf[MB_SER_ADU_ADDR_OFFSET];
        aduFrame->functionCode = pDev->AduBuf[MB_SER_ADU_PDU_OFFSET + MB_PDU_FUNCODE_OFF];
        /* Total length of Modbus-PDU is Modbus-Serial-Line-PDU minus
         * size of address field and CRC checksum.
         */
        aduFrame->pduFrameLength = (uint16_t) (pDev->rcvAduBuffPos - MB_SER_ADU_SIZE_ADDR - MB_SER_ADU_SIZE_CRC);

        /* Return the start of the Modbus PDU to the caller. */
        aduFrame->pPduFrame = (uint8_t *) &pDev->AduBuf[MB_SER_ADU_PDU_OFFSET];
    } else {
        status = MB_EIO;
    }
    EXIT_CRITICAL_SECTION();

    return status;
}

MbErrorCode_t MbsRTUSend(MbsHandle_t dev, uint8_t slaveID, const uint8_t *pPdu, uint16_t len) {
    MbErrorCode_t status = MB_ENOERR;
    uint16_t crcValue;
    uint8_t *pAdu;
    MbsDev_t *pDev = (MbsDev_t *) dev;

    ENTER_CRITICAL_SECTION();
    /* Check if the receiver is still in idle state. If not we where to
     * slow with processing the received frame and the master sent another
     * frame on the network. We have to abort sending the frame.
     */
    if (pDev->sendRcvState == STATE_RTU_RX_IDLE) {
        /* First byte before the Modbus-PDU is the slave address. */
        pAdu = (uint8_t *) pPdu - 1;
        pDev->sendAduBuffCount = 1;

        /* Now copy the Modbus-PDU into the Modbus-Serial-Line-PDU. */
        pAdu[MB_SER_ADU_ADDR_OFFSET] = slaveID;
        pDev->sendAduBuffCount += len;

        /* Calculate CRC16 checksum for Modbus-Serial-Line-PDU. */
        crcValue = MbCRC16((uint8_t *) pAdu, pDev->sendAduBuffCount);
        pDev->AduBuf[pDev->sendAduBuffCount++] = (uint8_t) (crcValue & 0xFF);
        pDev->AduBuf[pDev->sendAduBuffCount++] = (uint8_t) (crcValue >> 8);

        /* Activate the transmitter. */
        pDev->sendRcvState = STATE_RTU_TX_XMIT;

        /* start the first transmitter then into serial tc interrupt */
        MbPortSerialPutByte(pDev->port, pAdu[0]);
        pDev->sendAduBuffPos = 1;  /* next byte in sendBuffer. */
        pDev->sendAduBuffCount--;

        MbPortSerialEnable(pDev->port, false, true);
    } else {
        status = MB_EIO;
    }
    EXIT_CRITICAL_SECTION();

    return status;
}

void MbsRTUReceiveFSM(MbsHandle_t dev) {
    uint8_t byte;
    MbsDev_t *pDev = (MbsDev_t *) dev;

    /* Always read the character. */
    (void) MbPortSerialGetByte(pDev->port, (char *) &byte);

    /* In the idle state we wait for a new character. If a character
     * is received the t1.5 and t3.5 timers are started and the
     * receiver is in the state STATE_RX_RECEIVCE.
     */
    if (pDev->sendRcvState == STATE_RTU_RX_IDLE) {
        pDev->rcvAduBuffPos = 0;
        pDev->AduBuf[pDev->rcvAduBuffPos++] = byte;
        pDev->sendRcvState = STATE_RTU_RX_RCV;

    } else if (pDev->sendRcvState == STATE_RTU_RX_RCV) {
        /* We are currently receiving a frame. Reset the timer after
         * every character received. If more than the maximum possible
         * number of bytes in a modbus frame is received the frame is
         * ignored.
         */
        if (pDev->rcvAduBuffPos < MB_ADU_SIZE_MAX) {
            pDev->AduBuf[pDev->rcvAduBuffPos++] = byte;
        } else {
            /* In the error state we wait until all characters in the
             * damaged frame are transmitted.
             */
        }
    }

    /* Enable t3.5 timers. */
    MbPortTimersEnable(pDev->port);
}

void MbsRTUTransmitFSM(MbsHandle_t dev) {
    MbsDev_t *pDev = (MbsDev_t *) dev;

    /* We should get a transmitter event in transmitter state.  */
    if (pDev->sendRcvState == STATE_RTU_TX_XMIT) {
        /* check if we are finished. */
        if (pDev->sendAduBuffCount != 0) {
            MbPortSerialPutByte(pDev->port, (char) pDev->AduBuf[pDev->sendAduBuffPos]);
            pDev->sendAduBuffPos++;  /* next byte in sendbuffer. */
            pDev->sendAduBuffCount--;
        } else {
            /* Disable transmitter. This prevents another transmit buffer
             * empty interrupt. */
            MbPortSerialEnable(pDev->port, true, false);
            pDev->sendRcvState = STATE_RTU_RX_IDLE;
        }
    } else {
        /* enable receiver/disable transmitter. */
        MbPortSerialEnable(pDev->port, true, false);
    }
}

void MbsRTUTimerT35Expired(MbsHandle_t dev) {
    MbsDev_t *pDev = (MbsDev_t *) dev;

    /* A frame was received and t35 expired. Notify the listener that
     * a new frame was received. */
    if (pDev->sendRcvState == STATE_RTU_RX_RCV)
        MbsSemGive(pDev);

    MbPortTimersDisable(pDev->port);
    pDev->sendRcvState = STATE_RTU_RX_IDLE;
}

#endif

