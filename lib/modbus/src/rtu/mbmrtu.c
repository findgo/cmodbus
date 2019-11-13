
#include "port.h"
#include "mbrtu.h"
#include "mbutils.h"

/*************************************************************************************************/
/* TODO implement modbus rtu master */
#if MB_RTU_ENABLED > 0 && MB_MASTER_ENABLED > 0

MbErrorCode_t MbmRTUInit(MbmHandle_t dev, uint8_t ucPort, uint32_t ulBaudRate, MbParity_t eParity) {
    MbErrorCode_t eStatus = MB_ENOERR;
    uint32_t usTimerT35_50us;
    (void) dev;

    ENTER_CRITICAL_SECTION();
    /* Modbus RTU uses 8 Databits. */
    if (MbPortSerialInit(ucPort, ulBaudRate, 8, eParity) != true) {
        eStatus = MB_EPORTERR;
    } else {
        /* If baudrate > 19200 then we should use the fixed timer values
         * t35 = 1750us. Otherwise t35 must be 3.5 times the character time.
         */
        if (ulBaudRate > 19200) {
            usTimerT35_50us = 35;       /* 1800us. */
        } else {
            /* The timer reload value for a character is given by:
             *
             * ChTimeValue = Ticks_per_1s / ( Baudrate / 11 )
             *             = 11 * Ticks_per_1s / Baudrate
             *             = 220000 / Baudrate
             * The reload for t3.5 is 1.5 times this value and similary
             * for t3.5.
             */
            usTimerT35_50us = (7UL * 220000UL) / (2UL * ulBaudRate);
        }
        if (MbPortTimersInit(ucPort, (uint16_t) usTimerT35_50us) != true) {
            eStatus = MB_EPORTERR;
        }
    }
    EXIT_CRITICAL_SECTION();

    return eStatus;
}

void MbmRTUStart(MbmHandle_t dev) {
    ENTER_CRITICAL_SECTION();

    ((MbmDev_t *) dev)->sendRcvState = STATE_RTU_RX_IDLE;
    MbPortSerialEnable(((MbmDev_t *) dev)->port, true, false);
    MbPortTimersDisable(((MbmDev_t *) dev)->port);

    EXIT_CRITICAL_SECTION();
}


void MbmRTUStop(MbmHandle_t dev) {
    ENTER_CRITICAL_SECTION();
    MbPortSerialEnable(((MbmDev_t *) dev)->port, false, false);
    MbPortTimersDisable(((MbmDev_t *) dev)->port);
    EXIT_CRITICAL_SECTION();
}

void MbmRTUClose(void *dev) {


}

MbReqResult_t
MbmRTUReceive(MbmHandle_t dev, MbHeader_t *phead, uint8_t *pfunCode, uint8_t **premain, uint16_t *premainLength) {
    MbReqResult_t result = MBR_ENOERR;
    MbmDev_t *pdev = (MbmDev_t *) dev;

    ENTER_CRITICAL_SECTION();
    /* Length and CRC check */
    if (pdev->rcvAduBuffPos >= (MB_ADU_RTU_SIZE_MIN + 1) /* addr+funcode+(other >= 1)+crc(2)  */
        && (MbCRC16((uint8_t *) pdev->AduBuf, pdev->rcvAduBuffPos) == 0)) {

        phead->introute.slaveID = pdev->AduBuf[MB_SER_ADU_ADDR_OFFSET];
        /* Save the address field. All frames are passed to the upper layed
         * and the decision if a frame is used is done there.
         */
        *pfunCode = pdev->AduBuf[MB_SER_ADU_PDU_OFFSET + MB_PDU_FUNCODE_OFF];

        /* Total length of Modbus-PDU is Modbus-Serial-Line-PDU minus
         * size of address field and CRC checksum.
         */
        *premainLength = (uint16_t) (pdev->rcvAduBuffPos - MB_SER_ADU_SIZE_ADDR - MB_PDU_SIZE_FUNCODE -
                                     MB_SER_ADU_SIZE_CRC);

        /* Return the start of the Modbus PDU to the caller. */
        *premain = (uint8_t *) &pdev->AduBuf[MB_SER_ADU_PDU_OFFSET + MB_PDU_DATA_OFF];
    } else if (pdev->rcvAduBuffPos < (MB_ADU_RTU_SIZE_MIN + 1)) {
        result = MBR_MISSBYTE;
    } else {
        result = MBR_ECHECK;
    }
    EXIT_CRITICAL_SECTION();

    return result;
}

MbReqResult_t MbmRTUSend(MbmHandle_t dev, const uint8_t *pAdu, uint16_t usAduLength) {
    MbReqResult_t result = MBR_ENOERR;
    MbmDev_t *pdev = (MbmDev_t *) dev;

    ENTER_CRITICAL_SECTION();
    /* Check if the receiver is still in idle state. If not we where to
     * slow with processing the received frame and the master sent another
     * frame on the network. We have to abort sending the frame.
     */
    if (pdev->sendRcvState == STATE_RTU_RX_IDLE) {
        // copy to sendbuff
        pdev->sendAduBuffCount = usAduLength;
        memcpy((uint8_t *) pdev->AduBuf, pAdu, usAduLength);

        /* Activate the transmitter. */
        pdev->sendRcvState = STATE_RTU_TX_XMIT;

        /* start the first transmitter then into serial tc interrupt */
        MbPortSerialPutByte(pdev->port, pAdu[0]);
        pdev->sendAduBuffPos = 1;  /* next byte in sendbuffer. */
        pdev->sendAduBuffCount--;

        MbPortSerialEnable(pdev->port, false, true);
    } else {
        result = MBR_BUSY;
    }
    EXIT_CRITICAL_SECTION();

    return result;
}

void MbmRTUReceiveFSM(MbmHandle_t dev) {
    uint8_t ucByte;
    MbmDev_t *pdev = (MbmDev_t *) dev;

    /* Always read the character. */
    (void) MbPortSerialGetByte(pdev->port, (char *) &ucByte);

    /* In the idle state we wait for a new character. If a character
     * is received the t1.5 and t3.5 timers are started and the
     * receiver is in the state STATE_RX_RECEIVCE.
     */
    if (pdev->sendRcvState == STATE_RTU_RX_IDLE) {
        pdev->rcvAduBuffPos = 0;
        pdev->AduBuf[pdev->rcvAduBuffPos++] = ucByte;
        pdev->sendRcvState = STATE_RTU_RX_RCV;

    } else if (pdev->sendRcvState == STATE_RTU_RX_RCV) {
        /* We are currently receiving a frame. Reset the timer after
         * every character received. If more than the maximum possible
         * number of bytes in a modbus frame is received the frame is
         * ignored.
         */
        if (pdev->rcvAduBuffPos < MB_ADU_SIZE_MAX) {
            pdev->AduBuf[pdev->rcvAduBuffPos++] = ucByte;
        } else {
            /* In the error state we wait until all characters in the
             * damaged frame are transmitted.
             */
        }
    }

    /* Enable t3.5 timers. */
    MbPortTimersEnable(pdev->port);
}


void MbmRTUTransmitFSM(MbmHandle_t dev) {
    MbmDev_t *pdev = (MbmDev_t *) dev;

    /* We should get a transmitter event in transmitter state.  */
    if (pdev->sendRcvState == STATE_RTU_TX_XMIT) {
        /* check if we are finished. */
        if (pdev->sendAduBuffCount != 0) {
            MbPortSerialPutByte(pdev->port, (char) pdev->AduBuf[pdev->sendAduBuffPos]);
            pdev->sendAduBuffPos++;  /* next byte in sendbuffer. */
            pdev->sendAduBuffCount--;
        } else {
            /* Disable transmitter. This prevents another transmit buffer empty interrupt. */
            MbPortSerialEnable(pdev->port, true, false);
            pdev->sendRcvState = STATE_RTU_RX_IDLE;
        }
    } else {
        /* enable receiver/disable transmitter. */
        MbPortSerialEnable(pdev->port, true, false);
    }
}

void MbmRTUTimerT35Expired(MbmHandle_t dev) {
    MbmDev_t *pdev = (MbmDev_t *) dev;

    /* A frame was received and t35 expired. Notify the listener that
     * a new frame was received. */
    if (pdev->sendRcvState == STATE_RTU_RX_RCV && pdev->Pollstate == MBM_WAITRSP);
    MbmSetPollmode(pdev, MBM_RSPEXCUTE);

    MbPortTimersDisable(pdev->port);
    pdev->sendRcvState = STATE_RTU_RX_IDLE;
}

#endif

