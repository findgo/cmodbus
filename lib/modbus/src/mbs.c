#include "port.h"
#include "mb.h"
#include "mbfunc.h"
#include "mbutils.h"

#if MB_RTU_ENABLED > 0
#include "mbrtu.h"
#endif

#if MB_ASCII_ENABLED > 0
#include "mbascii.h"
#endif

#if MB_TCP_ENABLED > 0
#include "mbtcp.h"
#endif

#if (MB_RTU_ENABLED > 0 || MB_ASCII_ENABLED > 0) && MB_SLAVE_ENABLED > 0

// local variate 
static MbsDev_t mbs_devTal[MBS_SUPPORT_MULTIPLE_NUMBER];

//local function
static MbErrorCode_t __MbsAduFramehandle(MbsDev_t *dev);


MbsDev_t *MbsNew(MbMode_t eMode, uint8_t ucSlaveAddress, uint8_t ucPort, uint32_t ulBaudRate, MbParity_t eParity )
{
    MbsDev_t *dev = NULL;
    MbErrorCode_t eStatus;
    uint8_t i;

    /* check preconditions */
    if((ucSlaveAddress == MB_ADDRESS_BROADCAST) || ( ucSlaveAddress < MB_ADDRESS_MIN ) \
            || ( ucSlaveAddress > MB_ADDRESS_MAX )){        
        return NULL;
    }

    // search device table and then check port exit and dev in use ?
    for( i = 0; i < MBS_SUPPORT_MULTIPLE_NUMBER; i++ ){
        if( mbs_devTal[i].inuse == 0 ){
            dev = (MbsDev_t *)&mbs_devTal[i];
            break;
        }
        // find port in used 
        else if(mbs_devTal[i].port == ucPort){
            return NULL;
        }
    }
    // no space in device table
    if(i >= MBS_SUPPORT_MULTIPLE_NUMBER)
        return NULL;

    // find space to go on
    memset(dev, 0, sizeof(MbsDev_t));
    
    switch (eMode){
#if MB_RTU_ENABLED > 0
    case MB_RTU:
        dev->pMbStartCur = MbsRTUStart;
        dev->pMbStopCur = MbsRTUStop;
        dev->pMbCloseCur = MbsRTUClose;
        dev->pMbSendCur = MbsRTUSend;
        dev->pMbReceivedCur = MbsRTUReceive;

        eStatus = MbsRTUInit(dev, ucPort, ulBaudRate, eParity);
        break;
#endif

#if MB_ASCII_ENABLED > 0
    case MB_ASCII:
        dev->pMbStartCur = MbsASCIIStart;
        dev->pMbStopCur = MbsASCIIStop;
        dev->pMbCloseCur = MbsASCIIClose;
        dev->pMbSendCur = MbsASCIISend;
        dev->pMbReceivedCur = MbsASCIIReceive;
        
        eStatus = MbsASCIIInit(dev, ucPort, ulBaudRate, eParity);
        break;
#endif

    default:
        eStatus = MB_EINVAL;
        break;
    }

    // init failed
    if(eStatus != MB_ENOERR){
        return NULL;
    }
    
    dev->inuse = 1;             // mark it in use!
    dev->port = ucPort;
    dev->slaveaddr = ucSlaveAddress;  /* set slave address */
    dev->mode = eMode;
    dev->devstate = DEV_STATE_DISABLED;    
    dev->eventInFlag = FALSE;

    return dev;
}

void MbsFree(uint8_t ucPort)
{
    uint8_t i;
    
    for( i = 0; i < MBS_SUPPORT_MULTIPLE_NUMBER; i++ ){
        if( ( mbs_devTal[i].inuse == 1  ) && ( mbs_devTal[i].port == ucPort ) ){        
            mbs_devTal[i].inuse = 0; // mark it not in use!
            break;
        }
    }
}

//__align(2)  
//static uint8_t regbuf[REG_COILS_SIZE / 8 + REG_DISCRETE_SIZE / 8 + REG_INPUT_NREGS * 2 + REG_HOLDING_NREGS * 2];
MbErrorCode_t MbsRegAssign(MbsDev_t *dev,
                                uint8_t *regstoragebuf,  
                                uint32_t regstoragesize, 
                                uint16_t reg_holding_addr_start,
                                uint16_t reg_holding_num,
                                uint16_t reg_input_addr_start,
                                uint16_t reg_input_num,
                                uint16_t reg_coils_addr_start,
                                uint16_t reg_coils_num,
                                uint16_t reg_discrete_addr_start,
                                uint16_t reg_discrete_num)
{
    uint32_t offset;
    MbReg_t *regs;

    if(dev == NULL || regstoragebuf == NULL)
        return MB_EINVAL;

    if( regstoragesize < MbRegBufSizeCal( reg_holding_num, reg_input_num, reg_coils_num, reg_discrete_num ) )
        return MB_EINVAL;

    regs = (MbReg_t *)&(dev->regs);

    regs->reg_holding_addr_start = reg_holding_addr_start;
    regs->reg_holding_num = reg_holding_num;
    
    regs->reg_input_addr_start = reg_input_addr_start;    
    regs->reg_input_num = reg_input_num;
        
    regs->reg_coils_addr_start = reg_coils_addr_start;
    regs->reg_coils_num = reg_coils_num;
    
    regs->reg_discrete_addr_start = reg_discrete_addr_start;
    regs->reg_discrete_num = reg_discrete_num;

    // hold register
    regs->pReghold = (uint16_t *)&regstoragebuf[0];
    offset = reg_holding_num * sizeof(uint16_t);
    // input register
    regs->pReginput = (uint16_t *)&regstoragebuf[offset];
    offset +=  reg_input_num * sizeof(uint16_t);
    // coil register
    regs->pRegCoil = &regstoragebuf[offset];
    offset += ( reg_coils_num >> 3 ) + (((reg_coils_num & 0x07) > 0) ? 1 : 0);
    // disc register
    regs->pRegDisc = &regstoragebuf[offset];
    //offset += (reg_discrete_num >> 3) + (((reg_discrete_num & 0x07) > 0) ? 1 : 0);
    
    return MB_ENOERR;
}

MbErrorCode_t MbsRegAssignSingle(MbsDev_t *dev,
                                uint16_t *reg_holdingbuf,  
                                uint16_t reg_holding_addr_start,
                                uint16_t reg_holding_num,
                                uint16_t *reg_inputbuf,  
                                uint16_t reg_input_addr_start,
                                uint16_t reg_input_num,
                                uint8_t *reg_coilsbuf,  
                                uint16_t reg_coils_addr_start,
                                uint16_t reg_coils_num,
                                uint8_t *reg_discretebuf,  
                                uint16_t reg_discrete_addr_start,
                                uint16_t reg_discrete_num)
{
    MbReg_t *regs;

    if( dev == NULL )
        return MB_EINVAL;

    regs = (MbReg_t *)&(dev->regs);

    regs->reg_holding_addr_start = reg_holding_addr_start;
    regs->reg_holding_num = reg_holding_num;
    regs->pReghold = reg_holdingbuf;
    
    regs->reg_input_addr_start = reg_input_addr_start;    
    regs->reg_input_num = reg_input_num;
    regs->pReginput = reg_inputbuf;
    
    regs->reg_coils_addr_start = reg_coils_addr_start;
    regs->reg_coils_num = reg_coils_num;
    regs->pRegCoil = reg_coilsbuf;
    
    regs->reg_discrete_addr_start = reg_discrete_addr_start;
    regs->reg_discrete_num = reg_discrete_num;
    regs->pRegDisc = reg_discretebuf;
    
    return MB_ENOERR;
}

MbErrorCode_t MbsStart(MbsDev_t *dev)
{
    if( dev->devstate == DEV_STATE_NOT_INITIALIZED )
        return MB_EILLSTATE;

    if( dev->devstate == DEV_STATE_DISABLED ){
        /* Activate the protocol stack. */
        dev->pMbStartCur(dev);
        dev->devstate = DEV_STATE_ENABLED;
    }

    return MB_ENOERR;
}

MbErrorCode_t MbsStop(MbsDev_t *dev)
{
    if( dev->devstate == DEV_STATE_NOT_INITIALIZED )
        return MB_EILLSTATE;

    if( dev->devstate == DEV_STATE_ENABLED ){
        dev->pMbStopCur(dev);
        dev->devstate = DEV_STATE_DISABLED;
    }

    return MB_ENOERR;
}

MbErrorCode_t MbsClose(MbsDev_t *dev)
{
    // must be stop first then it can close
    if( dev->devstate == DEV_STATE_DISABLED ){
        if( dev->pMbCloseCur != NULL ){
            dev->pMbCloseCur(dev);
        }

        return MB_ENOERR;
    }
    
    return MB_EILLSTATE;
}

void MbsPoll(void)
{
    uint8_t i;

    for(i = 0; i < MBS_SUPPORT_MULTIPLE_NUMBER; i++){
        if(mbs_devTal[i].inuse) {
            ( void )__MbsAduFramehandle(&mbs_devTal[i]);
        }
    }
}

static MbErrorCode_t __MbsAduFramehandle(MbsDev_t *dev)
{
    uint8_t *pPduFrame; // pdu fram
    uint8_t ucRcvAddress;
    uint8_t ucFunctionCode;
    uint16_t usLength;
    MbException_t eException;

    pMbsFunctionHandler handle;
    MbErrorCode_t eStatus = MB_ENOERR;

    /* Check if the protocol stack is ready. */
    if( dev->devstate != DEV_STATE_ENABLED ){
        return MB_EILLSTATE;
    }

    /* Check if there is a event available. If not return control to caller.
     * Otherwise we will handle the event. */
    if(dev->eventInFlag){
        dev->eventInFlag = FALSE;
        /* parser a adu fram */
        eStatus = dev->pMbReceivedCur(dev, &ucRcvAddress, &pPduFrame, &usLength );
        if( eStatus != MB_ENOERR )
            return eStatus;
        
        /* Check if the frame is for us. If not ignore the frame. */
        if((ucRcvAddress == dev->slaveaddr) || (ucRcvAddress == MB_ADDRESS_BROADCAST) ){
            ucFunctionCode = pPduFrame[MB_PDU_FUNCODE_OFF];
            eException = MB_EX_ILLEGAL_FUNCTION;
            handle = MbsFuncHandleSearch(ucFunctionCode);
            if(handle)
                eException = handle(&dev->regs, pPduFrame, &usLength);
            
            /* If the request was not sent to the broadcast address and then we return a reply. */
            if(ucRcvAddress == MB_ADDRESS_BROADCAST)
                return MB_ENOERR;

            if(eException != MB_EX_NONE){
                /* An exception occured. Build an error frame. */
                usLength = 0;
                pPduFrame[usLength++] = (uint8_t)(ucFunctionCode | MB_FUNC_ERROR);
                pPduFrame[usLength++] = eException;
            }
            
            if((dev->mode == MB_ASCII) && MBS_ASCII_TIMEOUT_WAIT_BEFORE_SEND_MS){
                MbPortTimersDelay(dev->port, MBS_ASCII_TIMEOUT_WAIT_BEFORE_SEND_MS );
            }        
            /* send a reply */
            (void)dev->pMbSendCur(dev,dev->slaveaddr, pPduFrame, usLength);
        }
    }
    
    return MB_ENOERR;
}

#endif

