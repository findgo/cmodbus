
#include "port.h"
#include "mbrtu.h"
#include "mbutils.h"

/*************************************************************************************************/
/* TODO implement modbus rtu master */
#if MB_RTU_ENABLED > 0 && MB_MASTER_ENABLED > 0
MbErrorCode_t MbmRTUInit(void *dev, uint8_t ucPort, uint32_t ulBaudRate, MbParity_t eParity )
{
    MbErrorCode_t eStatus = MB_ENOERR;
    uint32_t usTimerT35_50us;
    (void)dev;
    
    ENTER_CRITICAL_SECTION();
    /* Modbus RTU uses 8 Databits. */
    if( MbPortSerialInit( ucPort, ulBaudRate, 8, eParity ) != TRUE ){
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
        if( MbPortTimersInit(ucPort, (uint16_t)usTimerT35_50us ) != TRUE){
            eStatus = MB_EPORTERR;
        }
    }
    EXIT_CRITICAL_SECTION();

    return eStatus;
}

void MbmRTUStart(void *dev)
{
    ENTER_CRITICAL_SECTION();
    
    ((MbmDev_t *)dev)->sndrcvState = STATE_RTU_RX_IDLE;
    MbPortSerialEnable(((MbmDev_t *)dev)->port, TRUE, FALSE);
    MbPortTimersDisable(((MbmDev_t *)dev)->port);

    EXIT_CRITICAL_SECTION();
}


void MbmRTUStop(void *dev)
{
    ENTER_CRITICAL_SECTION();
    MbPortSerialEnable(((MbmDev_t *)dev)->port, FALSE, FALSE);
    MbPortTimersDisable(((MbmDev_t *)dev)->port);
    EXIT_CRITICAL_SECTION();
}
void MbmRTUClose(void *dev)
{


}

MbReqResult_t MbmRTUReceive(void *dev,MbHeader_t *phead,uint8_t *pfunCode, uint8_t **premain, uint16_t *premainLength)
{
    MbReqResult_t result = MBR_ENOERR;
    MbmDev_t *pdev = (MbmDev_t *)dev;

    ENTER_CRITICAL_SECTION();
    /* Length and CRC check */
    if( pdev->rcvAduBufPos >= (MB_ADU_RTU_SIZE_MIN + 1) /* addr+funcode+(other >= 1)+crc(2)  */
        && (MbCRC16((uint8_t *)pdev->AduBuf, pdev->rcvAduBufPos) == 0)){

        phead->introute.slaveid = pdev->AduBuf[MB_SER_ADU_ADDR_OFFSET];
        /* Save the address field. All frames are passed to the upper layed
         * and the decision if a frame is used is done there.
         */
        *pfunCode = pdev->AduBuf[MB_SER_ADU_PDU_OFFSET + MB_PDU_FUNCODE_OFF];

        /* Total length of Modbus-PDU is Modbus-Serial-Line-PDU minus
         * size of address field and CRC checksum.
         */
        *premainLength = (uint16_t)(pdev->rcvAduBufPos - MB_SER_ADU_SIZE_ADDR - MB_PDU_SIZE_FUNCODE - MB_SER_ADU_SIZE_CRC);

        /* Return the start of the Modbus PDU to the caller. */
        *premain = (uint8_t *) & pdev->AduBuf[MB_SER_ADU_PDU_OFFSET + MB_PDU_DATA_OFF];
    }
    else if(pdev->rcvAduBufPos < (MB_ADU_RTU_SIZE_MIN + 1)){
        result = MBR_MISSBYTE;
    }else{
        result = MBR_ECHECK;
    }
    EXIT_CRITICAL_SECTION();
    
    return result;
}

MbReqResult_t MbmRTUSend(void *dev,const uint8_t *pAdu, uint16_t usAduLength)
{
    MbReqResult_t result = MBR_ENOERR;
    MbmDev_t *pdev = (MbmDev_t *)dev;
    
    ENTER_CRITICAL_SECTION();
    /* Check if the receiver is still in idle state. If not we where to
     * slow with processing the received frame and the master sent another
     * frame on the network. We have to abort sending the frame.
     */
    if( pdev->sndrcvState == STATE_RTU_RX_IDLE ){
        // copy to sendbuff
        pdev->sndAduBufCount = usAduLength;
        memcpy((uint8_t *)pdev->AduBuf,pAdu,usAduLength);

        /* Activate the transmitter. */
		pdev->sndrcvState = STATE_RTU_TX_XMIT;
        
		/* start the first transmitter then into serial tc interrupt */
        MbPortSerialPutByte(pdev->port, pAdu[0]);
        pdev->sndAduBufPos = 1;  /* next byte in sendbuffer. */
        pdev->sndAduBufCount--;
     	
        MbPortSerialEnable(pdev->port, FALSE, TRUE);
    }
    else{
        result = MBR_BUSY;
    }  
    EXIT_CRITICAL_SECTION();

    return result;
}

void MbmRTUReceiveFSM(  MbmDev_t *dev)
{
    uint8_t ucByte;

    /* Always read the character. */
    ( void )MbPortSerialGetByte(dev->port, (char *)&ucByte);

    /* In the idle state we wait for a new character. If a character
     * is received the t1.5 and t3.5 timers are started and the
     * receiver is in the state STATE_RX_RECEIVCE.
     */
    if(dev->sndrcvState == STATE_RTU_RX_IDLE){
        dev->rcvAduBufPos = 0;
        dev->AduBuf[dev->rcvAduBufPos++] = ucByte;
        dev->sndrcvState = STATE_RTU_RX_RCV;

    }
    else if(dev->sndrcvState == STATE_RTU_RX_RCV){
        /* We are currently receiving a frame. Reset the timer after
         * every character received. If more than the maximum possible
         * number of bytes in a modbus frame is received the frame is
         * ignored.
         */
        if(dev->rcvAduBufPos < MB_ADU_SIZE_MAX){
            dev->AduBuf[dev->rcvAduBufPos++] = ucByte;
        }
        else{
            /* In the error state we wait until all characters in the
             * damaged frame are transmitted.
             */
        }
    }
    
    /* Enable t3.5 timers. */
    MbPortTimersEnable(dev->port);
}




void MbmRTUTransmitFSM(  MbmDev_t *dev)
{
    /* We should get a transmitter event in transmitter state.  */
    if(dev->sndrcvState == STATE_RTU_TX_XMIT){
        /* check if we are finished. */
        if( dev->sndAduBufCount != 0 ){
            MbPortSerialPutByte(dev->port, (char)dev->AduBuf[dev->sndAduBufPos]);
            dev->sndAduBufPos++;  /* next byte in sendbuffer. */
            dev->sndAduBufCount--;
        }
        else{
            /* Disable transmitter. This prevents another transmit buffer empty interrupt. */             
            MbPortSerialEnable(dev->port, TRUE, FALSE);
            dev->sndrcvState = STATE_RTU_RX_IDLE;
            if(dev->Pollstate == MBM_XMITING)
                MbmSetPollmode(dev, MBM_WAITRSP); // 发送完毕，进入等待应答
        }
    }
    else {
        /* enable receiver/disable transmitter. */
        MbPortSerialEnable(dev->port, TRUE, FALSE);
    }
}

void MbmRTUTimerT35Expired(  MbmDev_t *dev)
{
    /* A frame was received and t35 expired. Notify the listener that
     * a new frame was received. */
    if(dev->sndrcvState == STATE_RTU_RX_RCV && dev->Pollstate == MBM_WAITRSP);
        MbmSetPollmode(dev, MBM_RSPEXCUTE);

    MbPortTimersDisable(dev->port);
    dev->sndrcvState = STATE_RTU_RX_IDLE;
}

#endif

