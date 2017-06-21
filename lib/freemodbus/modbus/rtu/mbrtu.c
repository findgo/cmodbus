

#include "mbrtu.h"

#if MB_RTU_ENABLED > 0

/* ----------------------- Type definitions ---------------------------------*/
typedef enum
{
    STATE_RX_IDLE,              /*!< Receiver is in idle state. */
    STATE_RX_RCV,               /*!< Frame is beeing received. */
    STATE_TX_XMIT               /*!< RTU Transmitter is in transfer state. */
} eMBSndRcvState;    

/* ----------------------- Start implementation -----------------------------*/
mb_ErrorCode_t eMBRTUInit(void *dev, uint8_t ucPort, uint32_t ulBaudRate, mb_Parity_t eParity)
{
    mb_ErrorCode_t eStatus = MB_ENOERR;
    uint32_t usTimerT35_50us;
    (void)dev;
    
    ENTER_CRITICAL_SECTION();
    /* Modbus RTU uses 8 Databits. */
    if(xMBPortSerialInit(ucPort, ulBaudRate, 8, eParity) != true){
        eStatus = MB_EPORTERR;
    }
    else{
        /* If baudrate > 19200 then we should use the fixed timer values
         * t35 = 1750us. Otherwise t35 must be 3.5 times the character time.
         */
        if(ulBaudRate > 19200){
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
            usTimerT35_50us = (7UL * 220000UL) / (2UL * ulBaudRate);
        }
        if(xMBPortTimersInit(ucPort, (uint16_t)usTimerT35_50us) != true){
            eStatus = MB_EPORTERR;
        }
    }
    
    EXIT_CRITICAL_SECTION();

    return eStatus;
}

void vMBRTUStart(void *dev)
{
    ENTER_CRITICAL_SECTION();
    
    ((mb_Device_t *)dev)->sndrcvState = STATE_RX_IDLE;
    vMBPortSerialEnable(((mb_Device_t *)dev)->port, true, false);
    vMBPortTimersDisable(((mb_Device_t *)dev)->port);

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
    mb_ErrorCode_t eStatus = MB_ENOERR;
    mb_Device_t *dev = (mb_Device_t *)pdev;

    ENTER_CRITICAL_SECTION();
    /* Length and CRC check */
    if((dev->rcvAduBufrPos >= MB_ADU_RTU_SIZE_MIN)
        && (prvxMBCRC16( (uint8_t *)dev->AduBuf, dev->rcvAduBufrPos) == 0)){
        
        /* Save the address field. All frames are passed to the upper layed
         * and the decision if a frame is used is done there.
         */
        *pucRcvAddress = dev->AduBuf[MB_SER_ADU_ADDR_OFFSET];

        /* Total length of Modbus-PDU is Modbus-Serial-Line-PDU minus
         * size of address field and CRC checksum.
         */
        *pusLength = ( uint16_t )(dev->rcvAduBufrPos - MB_SER_ADU_SIZE_ADDR - MB_SER_ADU_SIZE_CRC);

        /* Return the start of the Modbus PDU to the caller. */
        *pPdu = ( uint8_t * ) & dev->AduBuf[MB_SER_ADU_PDU_OFFSET];

    }
    else{
        eStatus = MB_EIO;
    }
    EXIT_CRITICAL_SECTION();
    
    return eStatus;
}

mb_ErrorCode_t eMBRTUSend(void *pdev,uint8_t ucSlaveAddress, const uint8_t *pPdu, uint16_t usLength)
{
    mb_ErrorCode_t eStatus = MB_ENOERR;
    uint16_t usCRC16;
    uint8_t *pAdu;
    mb_Device_t *dev = (mb_Device_t *)pdev;
    
    ENTER_CRITICAL_SECTION();
    /* Check if the receiver is still in idle state. If not we where to
     * slow with processing the received frame and the master sent another
     * frame on the network. We have to abort sending the frame.
     */
    if(dev->sndrcvState == STATE_RX_IDLE){
        /* First byte before the Modbus-PDU is the slave address. */
        pAdu = (uint8_t *) pPdu - 1;
        dev->sndAduBufCount = 1;

        /* Now copy the Modbus-PDU into the Modbus-Serial-Line-PDU. */
        pAdu[MB_SER_ADU_ADDR_OFFSET] = ucSlaveAddress;
        dev->sndAduBufCount += usLength;

        /* Calculate CRC16 checksum for Modbus-Serial-Line-PDU. */
        usCRC16 = prvxMBCRC16((uint8_t *)pAdu, dev->sndAduBufCount);
        dev->AduBuf[dev->sndAduBufCount++] = ( uint8_t )(usCRC16 & 0xFF);
        dev->AduBuf[dev->sndAduBufCount++] = ( uint8_t )(usCRC16 >> 8);

        /* Activate the transmitter. */
		dev->sndrcvState = STATE_TX_XMIT;
        
		/* start the first transmitter then into serial tc interrupt */
        xMBPortSerialPutByte(dev->port, pAdu[0]);
        dev->sndAduBufPos = 1;  /* next byte in sendbuffer. */
        dev->sndAduBufCount--;
     	
        vMBPortSerialEnable(dev->port, false, true);
    }
    else{
        eStatus = MB_EIO;
    }  
    EXIT_CRITICAL_SECTION();

    return eStatus;
}

bool xMBRTUReceiveFSM(  mb_Device_t *dev)
{
    bool xTaskNeedSwitch = false;
    uint8_t ucByte;

    /* Always read the character. */
    ( void )xMBPortSerialGetByte(dev->port, (char *)&ucByte);

    /* In the idle state we wait for a new character. If a character
     * is received the t1.5 and t3.5 timers are started and the
     * receiver is in the state STATE_RX_RECEIVCE.
     */
    if(dev->sndrcvState == STATE_RX_IDLE){
        dev->rcvAduBufrPos = 0;
        dev->AduBuf[dev->rcvAduBufrPos++] = ucByte;
        dev->sndrcvState = STATE_RX_RCV;

    }
    else if(dev->sndrcvState == STATE_RX_RCV){
        /* We are currently receiving a frame. Reset the timer after
         * every character received. If more than the maximum possible
         * number of bytes in a modbus frame is received the frame is
         * ignored.
         */
        if(dev->rcvAduBufrPos < MB_ADU_SIZE_MAX){
            dev->AduBuf[dev->rcvAduBufrPos++] = ucByte;
        }
        else{
            /* In the error state we wait until all characters in the
             * damaged frame are transmitted.
             */
        }
    }
    
    /* Enable t3.5 timers. */
    vMBPortTimersEnable(dev->port);
    
    return xTaskNeedSwitch;
}

bool xMBRTUTransmitFSM(  mb_Device_t *dev)
{
    /* We should get a transmitter event in transmitter state.  */
    if(dev->sndrcvState == STATE_TX_XMIT){
        /* check if we are finished. */
        if( dev->sndAduBufCount != 0 ){
            xMBPortSerialPutByte(dev->port, (char)dev->AduBuf[dev->sndAduBufPos]);
            dev->sndAduBufPos++;  /* next byte in sendbuffer. */
            dev->sndAduBufCount--;
        }
        else{
            /* Disable transmitter. This prevents another transmit buffer
             * empty interrupt. */
            vMBPortSerialEnable(dev->port, true, false);
            dev->sndrcvState = STATE_RX_IDLE;
        }
    }
    else {
        /* enable receiver/disable transmitter. */
        vMBPortSerialEnable(dev->port, true, false);
    }

    return true;
}

bool xMBRTUTimerT35Expired(  mb_Device_t *dev)
{
    bool xNeedPoll = false;

    /* A frame was received and t35 expired. Notify the listener that
     * a new frame was received. */
    if(dev->sndrcvState == STATE_RX_RCV)
        xNeedPoll = xMBSemGive(dev);

    vMBPortTimersDisable(dev->port);
    dev->sndrcvState = STATE_RX_IDLE;

    return xNeedPoll;
}

/*************************************************************************************************/
/* TODO implement modbus rtu master */
#if MB_MASTER_ENABLE > 1

mb_ErrorCode_t eMBMasterRTUInit(void *dev, uint8_t ucPort, uint32_t ulBaudRate, mb_Parity_t eParity )
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

void vMBMasterRTUStart(void *dev)
{
    ENTER_CRITICAL_SECTION();
    /* Initially the receiver is in the state STATE_RX_INIT. we start
     * the timer and if no character is received within t3.5 we change
     * to STATE_RX_IDLE. This makes sure that we delay startup of the
     * modbus protocol stack until the bus is free.
     */
    ((mb_MasterDevice_t *)dev)->rcvState = STATE_RX_INIT;
    vMBPortSerialEnable(((mb_MasterDevice_t *)dev)->port, true, false );
    vMBPortTimersEnable(((mb_MasterDevice_t *)dev)->port);

    EXIT_CRITICAL_SECTION();
}

void vMBMasterRTUStop(void *dev)
{
    ENTER_CRITICAL_SECTION();
    vMBPortSerialEnable(((mb_MasterDevice_t *)dev)->port, false, false );
    vMBPortTimersDisable(((mb_MasterDevice_t *)dev)->port);
    EXIT_CRITICAL_SECTION();
}
void vMBMasterRTUClose(void *dev)
{



}

mb_ErrorCode_t eMBMasterRTUReceive(void *pdev,mb_header_t *phead,uint8_t *pfunCode, uint8_t **premain, uint16_t *premainLength)
{
    mb_ErrorCode_t eStatus = MB_ENOERR;
    mb_Device_t *dev = (mb_MasterDevice_t *)pdev;

    assert( dev->rcvAduBufrPos < MB_ADU_SIZE_MAX);

    ENTER_CRITICAL_SECTION();
    /* Length and CRC check */
    if((dev->rcvAduBufrPos >= MB_ADU_RTU_SIZE_MIN)
        && (prvxMBCRC16( (uint8_t *)dev->AduBuf, dev->rcvAduBufrPos) == 0)){

        phead->introute.slaveid = dev->AduBuf[MB_SER_ADU_ADDR_OFFSET];
        /* Save the address field. All frames are passed to the upper layed
         * and the decision if a frame is used is done there.
         */
        *pfunCode = dev->AduBuf[MB_SER_ADU_PDU_OFFSET + MB_PDU_FUNCODE_OFF];

        /* Total length of Modbus-PDU is Modbus-Serial-Line-PDU minus
         * size of address field and CRC checksum.
         */
        *premainLength = (uint16_t)(dev->rcvAduBufrPos - MB_SER_ADU_SIZE_ADDR - MB_PDU_SIZE_FUNCODE - MB_SER_ADU_SIZE_CRC);

        /* Return the start of the Modbus PDU to the caller. */
        *premain = (uint8_t *) & dev->AduBuf[MB_SER_ADU_PDU_OFFSET + MB_PDU_DATA_OFF];

    }
    else{
        eStatus = MB_EIO;
    }
    EXIT_CRITICAL_SECTION();
    
    return eStatus;
}

mb_ErrorCode_t eMBMasterRTUSend(void *pdev,const uint8_t *pAdu, uint16_t usAduLength)
{
    mb_ErrorCode_t eStatus = MB_ENOERR;
    mb_MasterDevice_t *dev = (mb_MasterDevice_t *)pdev;
    
    ENTER_CRITICAL_SECTION();
    /* Check if the receiver is still in idle state. If not we where to
     * slow with processing the received frame and the master sent another
     * frame on the network. We have to abort sending the frame.
     */
    if( dev->rcvState == STATE_RX_IDLE ){
        // copy to sendbuff
        dev->sndAduBufCount = usAduLength;
        pvMBmemcpy(dev->AduBuf,pAdu,usAduLength);

        /* Activate the transmitter. */
		dev->sndState = STATE_TX_XMIT;
        
		/* start the first transmitter then into serial tc interrupt */
        xMBPortSerialPutByte(dev->port, pAdu[0]);
        dev->sndAduBufPos = 1;  /* next byte in sendbuffer. */
        dev->sndAduBufCount--;
     	
        vMBPortSerialEnable(dev->port, false, true);
    }
    else{
        eStatus = MB_EIO;
    }  
    EXIT_CRITICAL_SECTION();

    return eStatus;
}

bool xMBMasterRTUReceiveFSM(  mb_MasterDevice_t *dev)
{
    bool xTaskNeedSwitch = false;
    uint8_t ucByte;

    /* Always read the character. */
    ( void )xMBPortSerialGetByte(dev->port, (char *)&ucByte);

    /* In the idle state we wait for a new character. If a character
     * is received the t1.5 and t3.5 timers are started and the
     * receiver is in the state STATE_RX_RECEIVCE.
     */
    if(dev->sndrcvState == STATE_RX_IDLE){
        dev->rcvAduBufrPos = 0;
        dev->AduBuf[dev->rcvAduBufrPos++] = ucByte;
        dev->sndrcvState = STATE_RX_RCV;

    }
    else if(dev->sndrcvState == STATE_RX_RCV){
        /* We are currently receiving a frame. Reset the timer after
         * every character received. If more than the maximum possible
         * number of bytes in a modbus frame is received the frame is
         * ignored.
         */
        if(dev->rcvAduBufrPos < MB_ADU_SIZE_MAX){
            dev->AduBuf[dev->rcvAduBufrPos++] = ucByte;
        }
        else{
            /* In the error state we wait until all characters in the
             * damaged frame are transmitted.
             */
        }
    }
    
    /* Enable t3.5 timers. */
    vMBPortTimersEnable(dev->port);
    
    return xTaskNeedSwitch;
}



bool xMBMasterRTUTransmitFSM(  mb_MasterDevice_t *dev)
{
    /* We should get a transmitter event in transmitter state.  */
    if(dev->sndrcvState == STATE_TX_XMIT){
        /* check if we are finished. */
        if( dev->sndAduBufCount != 0 ){
            xMBPortSerialPutByte(dev->port, (char)dev->AduBuf[dev->sndAduBufPos]);
            dev->sndAduBufPos++;  /* next byte in sendbuffer. */
            dev->sndAduBufCount--;
        }
        else{
            /* Disable transmitter. This prevents another transmit buffer
             * empty interrupt. */
            vMBPortSerialEnable(dev->port, true, false);
            dev->sndrcvState = STATE_RX_IDLE;
        }
    }
    else {
        /* enable receiver/disable transmitter. */
        vMBPortSerialEnable(dev->port, true, false);
    }

    return true;
}


bool xMBMasterRTUTimerT35Expired(  mb_MasterDevice_t *dev)
{
    bool xNeedPoll = false;

    /* A frame was received and t35 expired. Notify the listener that
     * a new frame was received. */
    if(dev->sndrcvState == STATE_RX_RCV)
        xNeedPoll = xMBSemGive(dev);

    vMBPortTimersDisable(dev->port);
    dev->sndrcvState = STATE_RX_IDLE;

    return xNeedPoll;
}


#endif

#endif

