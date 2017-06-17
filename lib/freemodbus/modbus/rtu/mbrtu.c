

#include "mbrtu.h"

/* ----------------------- Type definitions ---------------------------------*/
typedef enum
{
    STATE_RX_INIT,              /*!< Receiver is in initial state. */
    STATE_RX_IDLE,              /*!< Receiver is in idle state. */
    STATE_RX_RCV,               /*!< Frame is beeing received. */
    STATE_RX_ERROR              /*!< If the frame is invalid. */
} eMBRcvState;

typedef enum
{
    STATE_TX_IDLE,              /*!< RTU Transmitter is in idle state. */
    STATE_TX_XMIT               /*!< RTU Transmitter is in transfer state. */
} eMBSndState;

/* ----------------------- Start implementation -----------------------------*/
eMBErrorCode eMBRTUInit(void *dev, uint8_t ucPort, uint32_t ulBaudRate, eMBParity eParity )
{
    eMBErrorCode eStatus = MB_ENOERR;
    uint32_t usTimerT35_50us;
    (void)dev;
    
    ENTER_CRITICAL_SECTION();

    /* Modbus RTU uses 8 Databits. */
    if( xMBPortSerialInit( ucPort, ulBaudRate, 8, eParity ) != true ){
        eStatus = MB_EPORTERR;
    }
    else{
        /* If baudrate > 19200 then we should use the fixed timer values
         * t35 = 1750us. Otherwise t35 must be 3.5 times the character time.
         */
        if( ulBaudRate > 19200 ){
            usTimerT35_50us = 35;       /* 1800us. */
        }
        else{
            /* The timer reload value for a character is given by:
             *
             * ChTimeValue = Ticks_per_1s / ( Baudrate / 11 )
             *             = 11 * Ticks_per_1s / Baudrate
             *             = 220000 / Baudrate
             * The reload for t3.5 is 1.5 times this value and similary
             * for t3.5.
             */
            usTimerT35_50us = ( 7UL * 220000UL ) / ( 2UL * ulBaudRate );
        }
        if( xMBPortTimersInit(ucPort, (uint16_t)usTimerT35_50us ) != true){
            eStatus = MB_EPORTERR;
        }
    }
    EXIT_CRITICAL_SECTION();

    return eStatus;
}

void eMBRTUStart(void *dev)
{
    ENTER_CRITICAL_SECTION();
    /* Initially the receiver is in the state STATE_RX_INIT. we start
     * the timer and if no character is received within t3.5 we change
     * to STATE_RX_IDLE. This makes sure that we delay startup of the
     * modbus protocol stack until the bus is free.
     */
    ((mb_device_t *)dev)->rcvState = STATE_RX_INIT;
    vMBPortSerialEnable(((mb_device_t *)dev)->port, true, false );
    vMBPortTimersEnable(((mb_device_t *)dev)->port);

    EXIT_CRITICAL_SECTION();
}

void eMBRTUStop(void *dev)
{
    ENTER_CRITICAL_SECTION();
    vMBPortSerialEnable(((mb_device_t *)dev)->port, false, false );
    vMBPortTimersDisable(((mb_device_t *)dev)->port);
    EXIT_CRITICAL_SECTION();
}
void eMBRTUClose(void *dev)
{



}
eMBErrorCode eMBRTUReceive(void *pdev,uint8_t *pucRcvAddress, uint8_t **pPdu, uint16_t *pusLength)
{
    eMBErrorCode    eStatus = MB_ENOERR;
    mb_device_t *dev = (mb_device_t *)pdev;

    assert( dev->rcvAduBufrPos < MB_ADU_SIZE_MAX );

    ENTER_CRITICAL_SECTION();
    /* Length and CRC check */
    if( (dev->rcvAduBufrPos >= MB_ADU_RTU_SIZE_MIN)
        && (usMBCRC16( (uint8_t *)dev->AduBuf, dev->rcvAduBufrPos) == 0 ) ){
        
        /* Save the address field. All frames are passed to the upper layed
         * and the decision if a frame is used is done there.
         */
        *pucRcvAddress = dev->AduBuf[MB_SER_ADU_ADDR_OFFSET];

        /* Total length of Modbus-PDU is Modbus-Serial-Line-PDU minus
         * size of address field and CRC checksum.
         */
        *pusLength = ( uint16_t )( dev->rcvAduBufrPos - MB_SER_ADU_SIZE_ADDR - MB_SER_ADU_SIZE_CRC );

        /* Return the start of the Modbus PDU to the caller. */
        *pPdu = ( uint8_t * ) & dev->AduBuf[MB_SER_ADU_PDU_OFFSET];

    }
    else{
        eStatus = MB_EIO;
    }
    EXIT_CRITICAL_SECTION();
    
    return eStatus;
}

eMBErrorCode eMBRTUSend(void *pdev,uint8_t ucSlaveAddress, const uint8_t *pPdu, uint16_t usLength )
{
    eMBErrorCode eStatus = MB_ENOERR;
    uint16_t usCRC16;
    uint8_t *pAdu;
    mb_device_t *dev = (mb_device_t *)pdev;
    
    ENTER_CRITICAL_SECTION(  );
    /* Check if the receiver is still in idle state. If not we where to
     * slow with processing the received frame and the master sent another
     * frame on the network. We have to abort sending the frame.
     */
    if( dev->rcvState == STATE_RX_IDLE ){
        /* First byte before the Modbus-PDU is the slave address. */
        pAdu = (uint8_t *) pPdu - 1;
        dev->sndAduBufCount = 1;

        /* Now copy the Modbus-PDU into the Modbus-Serial-Line-PDU. */
        pAdu[MB_SER_ADU_ADDR_OFFSET] = ucSlaveAddress;
        dev->sndAduBufCount += usLength;

        /* Calculate CRC16 checksum for Modbus-Serial-Line-PDU. */
        usCRC16 = usMBCRC16( (uint8_t *)pAdu, dev->sndAduBufCount );
        dev->AduBuf[dev->sndAduBufCount++] = ( uint8_t )(usCRC16 & 0xFF);
        dev->AduBuf[dev->sndAduBufCount++] = ( uint8_t )(usCRC16 >> 8);

        /* Activate the transmitter. */
		dev->sndState = STATE_TX_XMIT;
        
		/* start the first transmitter then into serial tc interrupt */
        xMBPortSerialPutByte(dev->port, pAdu[0]);
        dev->sndAduBufPos = 1;  /* next byte in sendbuffer. */
        dev->sndAduBufCount--;
     	
        vMBPortSerialEnable(dev->port, false, true );
    }
    else{
        eStatus = MB_EIO;
    }
    
    EXIT_CRITICAL_SECTION(  );
    return eStatus;
}

bool xMBRTUReceiveFSM(  mb_device_t *dev)
{
    bool            xTaskNeedSwitch = false;
    uint8_t           ucByte;

    assert( dev->sndState == STATE_TX_IDLE );

    /* Always read the character. */
    ( void )xMBPortSerialGetByte(dev->port, (char *)&ucByte );

    switch (dev->rcvState) {
        /* If we have received a character in the init state we have to
         * wait until the frame is finished.
         */
    case STATE_RX_INIT:
        vMBPortTimersEnable(dev->port);
        break;

        /* In the error state we wait until all characters in the
         * damaged frame are transmitted.
         */
    case STATE_RX_ERROR:
        vMBPortTimersEnable(dev->port);
        break;

        /* In the idle state we wait for a new character. If a character
         * is received the t1.5 and t3.5 timers are started and the
         * receiver is in the state STATE_RX_RECEIVCE.
         */
    case STATE_RX_IDLE:
        dev->rcvAduBufrPos = 0;
        dev->AduBuf[dev->rcvAduBufrPos++] = ucByte;
        dev->rcvState = STATE_RX_RCV;

        /* Enable t3.5 timers. */
        vMBPortTimersEnable(dev->port);
        break;

        /* We are currently receiving a frame. Reset the timer after
         * every character received. If more than the maximum possible
         * number of bytes in a modbus frame is received the frame is
         * ignored.
         */
    case STATE_RX_RCV:
        if(dev->rcvAduBufrPos < MB_ADU_SIZE_MAX){
            dev->AduBuf[dev->rcvAduBufrPos++] = ucByte;
        }
        else{
            dev->rcvState = STATE_RX_ERROR;
        }
        vMBPortTimersEnable(dev->port);
        break;
    }
    
    return xTaskNeedSwitch;
}

bool xMBRTUTransmitFSM(  mb_device_t *dev)
{
    assert( dev->rcvState == STATE_RX_IDLE );

    switch (dev->sndState){
        /* We should not get a transmitter event if the transmitter is in
         * idle state.  */
    case STATE_TX_IDLE:
        /* enable receiver/disable transmitter. */
        vMBPortSerialEnable(dev->port, true, false );
        break;

    case STATE_TX_XMIT:
        /* check if we are finished. */
        if( dev->sndAduBufCount != 0 ){
            xMBPortSerialPutByte(dev->port, ( char )dev->AduBuf[dev->sndAduBufPos] );
            dev->sndAduBufPos++;  /* next byte in sendbuffer. */
            dev->sndAduBufCount--;
        }
        else{
            /* Disable transmitter. This prevents another transmit buffer
             * empty interrupt. */
            vMBPortSerialEnable(dev->port, true, false );
            dev->sndState = STATE_TX_IDLE;
        }
        break;
    }

    return true;
}

bool xMBRTUTimerT35Expired(  mb_device_t *dev)
{
    bool xNeedPoll = false;

    switch ( dev->rcvState )
    {
        /* Timer t35 expired. Startup phase is finished. */
    case STATE_RX_INIT:
        break;

        /* A frame was received and t35 expired. Notify the listener that
         * a new frame was received. */
    case STATE_RX_RCV:
        xNeedPoll = xMBEventPost(dev);
        break;

        /* An error occured while receiving the frame. */
    case STATE_RX_ERROR:
        break;

        /* Function called in an illegal state. */
    default:
        assert( ( dev->rcvState == STATE_RX_INIT ) ||
                ( dev->rcvState == STATE_RX_RCV ) || ( dev->rcvState == STATE_RX_ERROR ) );
    }

    vMBPortTimersDisable(dev->port);
    dev->rcvState = STATE_RX_IDLE;

    return xNeedPoll;
}
