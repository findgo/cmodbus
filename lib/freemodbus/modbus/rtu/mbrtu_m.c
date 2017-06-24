
#include "port.h"
#include "mbrtu.h"



/*************************************************************************************************/
/* TODO implement modbus rtu master */
#if MB_RTU_ENABLED > 0 && MB_MASTER_ENABLED > 0
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
    
    ((mb_Device_t *)dev)->sndrcvState = STATE_RTU_RX_IDLE;
    vMBPortSerialEnable(((mb_Device_t *)dev)->port, true, false);
    vMBPortTimersDisable(((mb_Device_t *)dev)->port);

    EXIT_CRITICAL_SECTION();
}


void vMBMasterRTUStop(void *dev)
{
    ENTER_CRITICAL_SECTION();
    vMBPortSerialEnable(((mb_MasterDevice_t *)dev)->port, false, false);
    vMBPortTimersDisable(((mb_MasterDevice_t *)dev)->port);
    EXIT_CRITICAL_SECTION();
}
void vMBMasterRTUClose(void *dev)
{



}

mb_ErrorCode_t eMBMasterRTUReceive(void *pdev,mb_header_t *phead,uint8_t *pfunCode, uint8_t **premain, uint16_t *premainLength)
{
    mb_ErrorCode_t eStatus = MB_ENOERR;
    mb_MasterDevice_t *dev = (mb_MasterDevice_t *)pdev;

    ENTER_CRITICAL_SECTION();
    /* Length and CRC check */
    if((dev->rcvAduBufrPos >= 3)
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
    if( dev->sndrcvState == STATE_RTU_RX_IDLE ){
        // copy to sendbuff
        dev->sndAduBufCount = usAduLength;
        pvMBmemcpy((uint8_t *)dev->AduBuf,pAdu,usAduLength);

        /* Activate the transmitter. */
		dev->sndrcvState = STATE_RTU_TX_XMIT;
        
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

void vMBMasterRTUReceiveFSM(  mb_MasterDevice_t *dev)
{
    uint8_t ucByte;

    /* Always read the character. */
    ( void )xMBPortSerialGetByte(dev->port, (char *)&ucByte);

    /* In the idle state we wait for a new character. If a character
     * is received the t1.5 and t3.5 timers are started and the
     * receiver is in the state STATE_RX_RECEIVCE.
     */
    if(dev->sndrcvState == STATE_RTU_RX_IDLE){
        dev->rcvAduBufrPos = 0;
        dev->AduBuf[dev->rcvAduBufrPos++] = ucByte;
        dev->sndrcvState = STATE_RTU_RX_RCV;

    }
    else if(dev->sndrcvState == STATE_RTU_RX_RCV){
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
}




void vMBMasterRTUTransmitFSM(  mb_MasterDevice_t *dev)
{
    /* We should get a transmitter event in transmitter state.  */
    if(dev->sndrcvState == STATE_RTU_TX_XMIT){
        /* check if we are finished. */
        if( dev->sndAduBufCount != 0 ){
            xMBPortSerialPutByte(dev->port, (char)dev->AduBuf[dev->sndAduBufPos]);
            dev->sndAduBufPos++;  /* next byte in sendbuffer. */
            dev->sndAduBufCount--;
        }
        else{
            /* Disable transmitter. This prevents another transmit buffer empty interrupt. */             
            vMBPortSerialEnable(dev->port, true, false);
            dev->sndrcvState = STATE_RTU_RX_IDLE;
            if(dev->Pollstate == MASTER_XMITING)
                vMBMasterSetPollmode(dev, MASTER_WAITRSP); // 发送完毕，进入等待应答
        }
    }
    else {
        /* enable receiver/disable transmitter. */
        vMBPortSerialEnable(dev->port, true, false);
    }
}

void vMBMasterRTUTimerT35Expired(  mb_MasterDevice_t *dev)
{
    /* A frame was received and t35 expired. Notify the listener that
     * a new frame was received. */
    if(dev->sndrcvState == STATE_RTU_RX_RCV && dev->Pollstate == MASTER_WAITRSP);
        vMBMasterSetPollmode(dev, MASTER_RSPEXCUTE);

    vMBPortTimersDisable(dev->port);
    dev->sndrcvState = STATE_RTU_RX_IDLE;
}

#endif

