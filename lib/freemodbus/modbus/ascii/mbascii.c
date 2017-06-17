
/* ----------------------- System includes ----------------------------------*/
#include "stdlib.h"
#include "string.h"

/* ----------------------- Platform includes --------------------------------*/
#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbconfig.h"
#include "mbascii.h"
#include "mbframe.h"

#include "mbcrc.h"
#include "mbport.h"

#if MB_ASCII_ENABLED > 0

/* ----------------------- Defines ------------------------------------------*/
#define MB_ASCII_DEFAULT_CR     '\r'    /*!< Default CR character for Modbus ASCII. */
#define MB_ASCII_DEFAULT_LF     '\n'    /*!< Default LF character for Modbus ASCII. */

/* ----------------------- Type definitions ---------------------------------*/
typedef enum
{
    STATE_RX_IDLE,              /*!< Receiver is in idle state. */
    STATE_RX_RCV,               /*!< Frame is beeing received. */
    STATE_RX_WAIT_EOF           /*!< Wait for End of Frame. */
} eMBRcvState;
typedef enum
{
    STATE_TX_IDLE,              /*!< ASCII Transmitter is in idle state. */
    STATE_TX_START,             /*!< ASCII Starting transmission (':' sent). */
    STATE_TX_DATA,              /*!< ASCII Sending of data (Address, Data, LRC). */
    STATE_TX_END,               /*!< ASCII End of transmission. */
    STATE_TX_NOTIFY,            /*!< ASCII Notify sender that the frame has been sent. */
} eMBSndState;



typedef enum
{
    BYTE_HIGH_NIBBLE,           /*!< Character for high nibble of byte. */
    BYTE_LOW_NIBBLE             /*!< Character for low nibble of byte. */
} eMBBytePos;

/* ----------------------- Static functions ---------------------------------*/
static uint8_t    prvucMBCHAR2BIN( uint8_t ucCharacter );
static uint8_t    prvucMBBIN2CHAR( uint8_t ucByte );
static uint8_t    prvucMBLRC( uint8_t * pPdu, uint16_t usLen );

static volatile eMBBytePos eBytePos;
static volatile uint8_t ucMBLFCharacter;

/* ----------------------- Start implementation -----------------------------*/
eMBErrorCode eMBASCIIInit(void *dev, uint8_t ucPort, uint32_t ulBaudRate, eMBParity eParity)
{
    eMBErrorCode eStatus = MB_ENOERR;
    (void)dev;
    
    ENTER_CRITICAL_SECTION(  );
    ucMBLFCharacter = MB_ASCII_DEFAULT_LF;

    if( xMBPortSerialInit( ucPort, ulBaudRate, 7, eParity ) != true ){
        eStatus = MB_EPORTERR;
    }
    else if( xMBPortTimersInit(ucPort, MB_ASCII_TIMEOUT_SEC * 20000UL ) != true ){
        eStatus = MB_EPORTERR;
    }
    EXIT_CRITICAL_SECTION(  );

    return eStatus;
}

void eMBASCIIStart(void *dev)
{
    ENTER_CRITICAL_SECTION(  );
    vMBPortSerialEnable((mb_device_t *)dev->port, true, false );
    (mb_device_t *)dev->rcvState = STATE_RX_IDLE;
    EXIT_CRITICAL_SECTION(  );
}

void eMBASCIIStop(void *dev)
{
    ENTER_CRITICAL_SECTION(  );
    vMBPortSerialEnable((mb_device_t *)dev->port, false, false );
    vMBPortTimersDisable((mb_device_t *)dev->port);
    EXIT_CRITICAL_SECTION(  );
}

eMBErrorCode eMBASCIIReceive(mb_device_t *pdev,uint8_t * pucRcvAddress, uint8_t **pPdu, uint16_t * pusLength)
{
    eMBErrorCode    eStatus = MB_ENOERR;
    mb_device_t *dev = (mb_device_t *)pdev;

    ENTER_CRITICAL_SECTION(  );
    assert( dev->rcvAduBufrPos < MB_ADU_SIZE_MAX );

    /* Length and LRC check */
    if( ( dev->rcvAduBufrPos >= MB_ADU_ASCII_SIZE_MIN )
        && ( prvucMBLRC( ( uint8_t * )dev->AduBuf, dev->rcvAduBufrPos ) == 0 ) ){
        /* Save the address field. All frames are passed to the upper layed
         * and the decision if a frame is used is done there.
         */
        *pucRcvAddress = dev->AduBuf[MB_SER_ADU_ADDR_OFFSET];

        /* Total length of Modbus-PDU is Modbus-Serial-Line-PDU minus
         * size of address field and CRC checksum.
         */
        *pusLength = ( uint16_t )( dev->rcvAduBufrPos - MB_SER_ADU_SIZE_ADDR - MB_SER_ADU_SIZE_LRC );

        /* Return the start of the Modbus PDU to the caller. */
        *pPdu = ( uint8_t * ) &dev->AduBuf[MB_SER_ADU_PDU_OFFSET];
    }
    else{
        eStatus = MB_EIO;
    }
    EXIT_CRITICAL_SECTION(  );
    
    return eStatus;
}

eMBErrorCode eMBASCIISend(mb_device_t *pdev, uint8_t ucSlaveAddress, const uint8_t * pPdu, uint16_t usLength )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    uint8_t           usLRC;
    uint8_t *pAdu;
    mb_device_t *dev = (mb_device_t *)pdev;
    
    ENTER_CRITICAL_SECTION(  );
    /* Check if the receiver is still in idle state. If not we where too
     * slow with processing the received frame and the master sent another
     * frame on the network. We have to abort sending the frame.
     */
    if( dev->rcvState == STATE_RX_IDLE ){
        
        /* First byte before the Modbus-PDU is the slave address. */
        pAdu = ( uint8_t * )pPdu - 1;
        dev->sndAduBufCount = 1;

        /* Now copy the Modbus-PDU into the Modbus-Serial-Line-PDU. */
        pAdu[MB_SER_ADU_ADDR_OFFSET] = ucSlaveAddress;
        dev->sndAduBufCount += usLength;

        /* Calculate LRC checksum for Modbus-Serial-Line-PDU. */
        usLRC = prvucMBLRC( ( uint8_t * ) pAdu, dev->sndAduBufCount );
        dev->AduBuf[dev->sndAduBufCount++] = usLRC;

        /* Activate the transmitter. */
        dev->sndState = STATE_TX_START;
        vMBPortSerialEnable(dev->port, false, true );
    }
    else{
        eStatus = MB_EIO;
    }
    EXIT_CRITICAL_SECTION(  );
    
    return eStatus;
}

bool xMBASCIIReceiveFSM(  mb_device_t *dev)
{
    bool            xNeedPoll = false;
    uint8_t           ucByte;
    uint8_t           ucResult;

    assert( dev->sndState == STATE_TX_IDLE );

    ( void )xMBPortSerialGetByte(dev->port, (char *) &ucByte );
    switch ( dev->rcvState ){
        /* A new character is received. If the character is a ':' the input
         * buffer is cleared. A CR-character signals the end of the data
         * block. Other characters are part of the data block and their
         * ASCII value is converted back to a binary representation.
         */
    case STATE_RX_RCV:
        /* Enable timer for character timeout. */
        vMBPortTimersEnable(dev->port);
        if( ucByte == ':' ){
            /* Empty receive buffer. */
            eBytePos = BYTE_HIGH_NIBBLE;
            dev->rcvAduBufrPos = 0;
        }
        else if( ucByte == MB_ASCII_DEFAULT_CR ){
            dev->rcvState = STATE_RX_WAIT_EOF;
        }
        else{
            ucResult = prvucMBCHAR2BIN( ucByte );
            switch ( eBytePos ){
                /* High nibble of the byte comes first. We check for
                 * a buffer overflow here. */
            case BYTE_HIGH_NIBBLE:
                if( dev->rcvAduBufrPos < MB_ADU_SIZE_MAX ){
                    dev->AduBuf[dev->rcvAduBufrPos] = ( uint8_t )( ucResult << 4 );
                    eBytePos = BYTE_LOW_NIBBLE;
                    break;
                }
                else{
                    /* not handled in Modbus specification but seems
                     * a resonable implementation. */
                    dev->rcvState = STATE_RX_IDLE;
                    /* Disable previously activated timer because of error state. */
                    vMBPortTimersDisable(dev->port);
                }
                break;

            case BYTE_LOW_NIBBLE:
                dev->AduBuf[dev->rcvAduBufrPos] |= ucResult;
                dev->rcvAduBufrPos++;
                eBytePos = BYTE_HIGH_NIBBLE;
                break;
            }
        }
        break;

    case STATE_RX_WAIT_EOF:
        if( ucByte == ucMBLFCharacter ){
            /* Disable character timeout timer because all characters are
             * received. */
            vMBPortTimersDisable(dev->port);
            /* Receiver is again in idle state. */
            dev->rcvState = STATE_RX_IDLE;

            /* Notify the caller of eMBASCIIReceive that a new frame was received. */
            xNeedPoll = xMBEventPost(dev, EV_FRAME_RECEIVED);
        }
        else if( ucByte == ':' ){
            /* Empty receive buffer and back to receive state. */
            eBytePos = BYTE_HIGH_NIBBLE;
            dev->rcvAduBufrPos = 0;
            dev->rcvState = STATE_RX_RCV;

            /* Enable timer for character timeout. */
            vMBPortTimersEnable(dev->port);
        }
        else{
            /* Frame is not okay. Delete entire frame. */
            dev->rcvState = STATE_RX_IDLE;
        }
        break;

    case STATE_RX_IDLE:
        if( ucByte == ':' ){
            /* Enable timer for character timeout. */
            vMBPortTimersEnable(dev->port);
            /* Reset the input buffers to store the frame. */
            dev->rcvAduBufrPos = 0;
            eBytePos = BYTE_HIGH_NIBBLE;
            dev->rcvState = STATE_RX_RCV;
        }
        break;
    }

    return xNeedPoll;
}

bool xMBASCIITransmitFSM(  mb_device_t *dev)
{
    bool xNeedPoll = false;
    uint8_t ucByte;

    assert( dev->rcvState == STATE_RX_IDLE );
    
    switch ( dev->sndState )
    {
        /* Start of transmission. The start of a frame is defined by sending
         * the character ':'. */
    case STATE_TX_START:
        ucByte = ':';
        xMBPortSerialPutByte( ( char )ucByte );
        dev->sndState = STATE_TX_DATA;
        eBytePos = BYTE_HIGH_NIBBLE;
        break;

        /* Send the data block. Each data byte is encoded as a character hex
         * stream with the high nibble sent first and the low nibble sent
         * last. If all data bytes are exhausted we send a '\r' character
         * to end the transmission. */
    case STATE_TX_DATA:
        if( dev->sndAduBufCount > 0 ){
            switch ( eBytePos ){
                
            case BYTE_HIGH_NIBBLE:
                ucByte = prvucMBBIN2CHAR( ( uint8_t )( dev->AduBuf[dev->sndAduBufPos] >> 4 ) );
                xMBPortSerialPutByte( (char) ucByte );
                eBytePos = BYTE_LOW_NIBBLE;
                break;

            case BYTE_LOW_NIBBLE:
                ucByte = prvucMBBIN2CHAR( ( uint8_t )( dev->AduBuf[dev->sndAduBufPos] & 0x0F ) );
                xMBPortSerialPutByte( (char)ucByte );
                dev->sndAduBufPos++;
                dev->sndAduBufCount--;                
                eBytePos = BYTE_HIGH_NIBBLE;
                break;
            }
        }
        else{
            xMBPortSerialPutByte( MB_ASCII_DEFAULT_CR );
            dev->sndState = STATE_TX_END;
        }
        break;

        /* Finish the frame by sending a LF character. */
    case STATE_TX_END:
        xMBPortSerialPutByte( (char)ucMBLFCharacter );
        /* We need another state to make sure that the CR character has
         * been sent. */
        dev->sndState = STATE_TX_NOTIFY;
        break;

        /* Notify the task which called eMBASCIISend that the frame has
         * been sent. */
    case STATE_TX_NOTIFY:
        dev->sndState = STATE_TX_IDLE;

        /* Disable transmitter. This prevents another transmit buffer
         * empty interrupt. */
        vMBPortSerialEnable(dev->port, true, false );
        dev->sndState = STATE_TX_IDLE;
        break;

        /* We should not get a transmitter event if the transmitter is in
         * idle state.  */
    case STATE_TX_IDLE:
        /* enable receiver/disable transmitter. */
        vMBPortSerialEnable(dev->port, true, false );
        break;
    }

    return xNeedPoll;
}

bool xMBASCIITimerT1SExpired(  mb_device_t *dev)
{
    switch (dev->rcvState ){
        /* If we have a timeout we go back to the idle state and wait for
         * the next frame.
         */
    case STATE_RX_RCV:
    case STATE_RX_WAIT_EOF:
        dev->rcvState = STATE_RX_IDLE;
        break;

    default:
        assert( ( dev->rcvState == STATE_RX_RCV ) || ( dev->rcvState == STATE_RX_WAIT_EOF ) );
        break;
    }
    vMBPortTimersDisable(dev->port);

    /* no context switch required. */
    return false;
}


static uint8_t prvucMBCHAR2BIN( uint8_t ucCharacter )
{
    if( ( ucCharacter >= '0' ) && ( ucCharacter <= '9' ) ){
        return ( uint8_t )( ucCharacter - '0' );
    }
    else if( ( ucCharacter >= 'A' ) && ( ucCharacter <= 'F' ) ){
        return ( uint8_t )( ucCharacter - 'A' + 0x0A );
    }
    else{
        return 0xFF;
    }
}

static uint8_t prvucMBBIN2CHAR( uint8_t ucByte )
{
    if( ucByte <= 0x09 ){
        return ( uint8_t )( '0' + ucByte );
    }
    else if( ( ucByte >= 0x0A ) && ( ucByte <= 0x0F ) ){
        return ( uint8_t )( ucByte - 0x0A + 'A' );
    }
    else{
        /* Programming error. */
        assert( 0 );
    }
    
    return '0';
}


static uint8_t prvucMBLRC( uint8_t * pPdu, uint16_t usLen )
{
    uint8_t ucLRC = 0;  /* LRC char initialized */

    while( usLen-- )
    {
        ucLRC += *pPdu++;   /* Add buffer byte without carry */
    }

    /* Return twos complement */
    ucLRC = ( uint8_t ) ( -((char) ucLRC ) );
    
    return ucLRC;
}

#endif
