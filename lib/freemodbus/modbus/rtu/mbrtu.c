

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
    
//private funciton
static uint16_t __prvxMBCRC16(uint8_t * pucFrame, uint16_t usLen);

/* ----------------------- Start implementation -----------------------------*/
mb_ErrorCode_t eMBRTUInit(void *dev, uint8_t ucPort, uint32_t ulBaudRate, mb_Parity_t eParity )
{
    mb_ErrorCode_t eStatus = MB_ENOERR;
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

void vMBRTUStart(void *dev)
{
    ENTER_CRITICAL_SECTION();
    /* Initially the receiver is in the state STATE_RX_INIT. we start
     * the timer and if no character is received within t3.5 we change
     * to STATE_RX_IDLE. This makes sure that we delay startup of the
     * modbus protocol stack until the bus is free.
     */
    ((mb_Device_t *)dev)->rcvState = STATE_RX_INIT;
    vMBPortSerialEnable(((mb_Device_t *)dev)->port, true, false );
    vMBPortTimersEnable(((mb_Device_t *)dev)->port);

    EXIT_CRITICAL_SECTION();
}

void vMBRTUStop(void *dev)
{
    ENTER_CRITICAL_SECTION();
    vMBPortSerialEnable(((mb_Device_t *)dev)->port, false, false );
    vMBPortTimersDisable(((mb_Device_t *)dev)->port);
    EXIT_CRITICAL_SECTION();
}
void vMBRTUClose(void *dev)
{



}
mb_ErrorCode_t eMBRTUReceive(void *pdev,uint8_t *pucRcvAddress, uint8_t **pPdu, uint16_t *pusLength)
{
    mb_ErrorCode_t    eStatus = MB_ENOERR;
    mb_Device_t *dev = (mb_Device_t *)pdev;

    assert( dev->rcvAduBufrPos < MB_ADU_SIZE_MAX );

    ENTER_CRITICAL_SECTION();
    /* Length and CRC check */
    if( (dev->rcvAduBufrPos >= MB_ADU_RTU_SIZE_MIN)
        && (__prvxMBCRC16( (uint8_t *)dev->AduBuf, dev->rcvAduBufrPos) == 0 ) ){
        
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

mb_ErrorCode_t eMBRTUSend(void *pdev,uint8_t ucSlaveAddress, const uint8_t *pPdu, uint16_t usLength )
{
    mb_ErrorCode_t eStatus = MB_ENOERR;
    uint16_t usCRC16;
    uint8_t *pAdu;
    mb_Device_t *dev = (mb_Device_t *)pdev;
    
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
        usCRC16 = __prvxMBCRC16( (uint8_t *)pAdu, dev->sndAduBufCount );
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

bool xMBRTUReceiveFSM(  mb_Device_t *dev)
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

bool xMBRTUTransmitFSM(  mb_Device_t *dev)
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

bool xMBRTUTimerT35Expired(  mb_Device_t *dev)
{
    bool xNeedPoll = false;

    switch (dev->rcvState)
    {
        /* Timer t35 expired. Startup phase is finished. */
    case STATE_RX_INIT:
        break;

        /* A frame was received and t35 expired. Notify the listener that
         * a new frame was received. */
    case STATE_RX_RCV:
        xNeedPoll = xMBSemGive(dev);
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

static uint16_t __prvxMBCRC16(uint8_t *pucFrame, uint16_t usLen)
{
    static const uint8_t aucCRCHi[] = {
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
        0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 
        0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40
    };
    
    static const uint8_t aucCRCLo[] = {
        0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7,
        0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E,
        0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09, 0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9,
        0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC,
        0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
        0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32,
        0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4, 0x3C, 0xFC, 0xFD, 0x3D,
        0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A, 0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 
        0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF,
        0x2D, 0xED, 0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
        0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 0x61, 0xA1,
        0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4,
        0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F, 0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 
        0x69, 0xA9, 0xA8, 0x68, 0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA,
        0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
        0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0,
        0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92, 0x96, 0x56, 0x57, 0x97,
        0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C, 0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E,
        0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89,
        0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
        0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83,
        0x41, 0x81, 0x80, 0x40
    };

    uint8_t ucCRCHi = 0xFF;
    uint8_t ucCRCLo = 0xFF;
    int iIndex;

    while( usLen-- )
    {
        iIndex = ucCRCLo ^ *( pucFrame++ );
        ucCRCLo = ( uint8_t )( ucCRCHi ^ aucCRCHi[iIndex] );
        ucCRCHi = aucCRCLo[iIndex];
    }
    
    return ( uint16_t )(ucCRCHi << 8 | ucCRCLo);
}

