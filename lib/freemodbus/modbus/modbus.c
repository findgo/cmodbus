

#include "modbus.h"
#include "mbfunc.h"

#if MB_RTU_ENABLED == 1
#include "mbrtu.h"
#endif
#if MB_ASCII_ENABLED == 1
#include "mbascii.h"
#endif
#if MB_TCP_ENABLED == 1
#include "mbtcp.h"
#endif

typedef struct
{
    uint8_t ucFunctionCode;
    pxMBFunctionHandler pxHandler;
} xMBFunctionHandler;
static mb_Device_t *__dev_search(uint8_t port);
static mb_ErrorCode_t __dev_add(mb_Device_t *dev);
static mb_ErrorCode_t eMBADUFramehandle(mb_Device_t *dev);

static mb_Device_t *mb_dev_head = NULL;

/* An array of Modbus functions handlers which associates Modbus function
 * codes with implementing functions.
 */
static xMBFunctionHandler xFuncHandlers[MB_FUNC_HANDLERS_MAX] = {
#if MB_FUNC_OTHER_REP_SLAVEID_ENABLED > 0
    {MB_FUNC_OTHER_REPORT_SLAVEID, eMBFuncReportSlaveID},
#endif
#if MB_FUNC_READ_HOLDING_ENABLED > 0
    {MB_FUNC_READ_HOLDING_REGISTER, eMBFuncReadHoldingRegister},
#endif
#if MB_FUNC_WRITE_HOLDING_ENABLED > 0
    {MB_FUNC_WRITE_REGISTER, eMBFuncWriteHoldingRegister},
#endif
#if MB_FUNC_WRITE_MULTIPLE_HOLDING_ENABLED > 0
        {MB_FUNC_WRITE_MULTIPLE_REGISTERS, eMBFuncWriteMultipleHoldingRegister},
#endif
#if MB_FUNC_READWRITE_HOLDING_ENABLED > 0
    {MB_FUNC_READWRITE_MULTIPLE_REGISTERS, eMBFuncReadWriteMultipleHoldingRegister},
#endif
#if MB_FUNC_READ_INPUT_ENABLED > 0
    {MB_FUNC_READ_INPUT_REGISTER, eMBFuncReadInputRegister},
#endif
#if MB_FUNC_READ_COILS_ENABLED > 0
    {MB_FUNC_READ_COILS, eMBFuncReadCoils},
#endif
#if MB_FUNC_WRITE_COIL_ENABLED > 0
    {MB_FUNC_WRITE_SINGLE_COIL, eMBFuncWriteCoil},
#endif
#if MB_FUNC_WRITE_MULTIPLE_COILS_ENABLED > 0
    {MB_FUNC_WRITE_MULTIPLE_COILS, eMBFuncWriteMultipleCoils},
#endif
#if MB_FUNC_READ_DISCRETE_INPUTS_ENABLED > 0
    {MB_FUNC_READ_DISCRETE_INPUTS, eMBFuncReadDiscreteInputs},
#endif
};

#if MB_DYNAMIC_MEMORY_ALLOC_ENABLE > 0

uint32_t xMBRegBufSizeCal(     uint16_t reg_holding_num,
                            uint16_t reg_input_num,
                            uint16_t reg_coils_num,
                            uint16_t reg_discrete_num)
{
    uint32_t size;

    size = reg_holding_num * sizeof(uint16_t);    
    size +=  reg_input_num * sizeof(uint16_t);
    size += (reg_coils_num >> 3) + (((reg_coils_num & 0x07) > 0) ? 1 : 0);
    size += (reg_discrete_num >> 3) + (((reg_discrete_num & 0x07) > 0) ? 1 : 0);

    return size;
}

mb_Device_t *xMBBaseDeviceNew(void)
{
    mb_Device_t *dev;
    
    dev = mb_malloc(sizeof(mb_Device_t));
    if(dev == NULL)
        return NULL;

    return dev;
}
uint8_t *xMBRegBufNew(uint32_t size)
{
    uint8_t *pregbuf;
    
    pregbuf = mb_malloc(size);
    if(pregbuf == NULL)
        return NULL;

    return pregbuf;    
}
#endif

#if MB_RTU_ENABLED > 0 || MB_ASCII_ENABLED
mb_ErrorCode_t eMBOpen(mb_Device_t *dev, mb_Mode_t eMode, uint8_t ucSlaveAddress, 
                        uint8_t ucPort, uint32_t ulBaudRate, mb_Parity_t eParity )
{
    mb_ErrorCode_t eStatus = MB_ENOERR;

    /* check preconditions */
    if((ucSlaveAddress == MB_ADDRESS_BROADCAST) \
        || ( ucSlaveAddress < MB_ADDRESS_MIN ) \
        || ( ucSlaveAddress > MB_ADDRESS_MAX ) ){
        
        return MB_EINVAL;
    }

    /* set slave address */
    dev->slaveid = ucSlaveAddress;
    dev->port = ucPort;
    switch (eMode){
#if MB_RTU_ENABLED > 0
    case MB_RTU:
        dev->pvMBStartCur = vMBRTUStart;
        dev->pvMBStopCur = vMBRTUStop;
        dev->pvMBCloseCur = vMBRTUClose;
        dev->peMBSendCur = eMBRTUSend;
        dev->peMBReceivedCur = eMBRTUReceive;

        eStatus = eMBRTUInit(dev, ucPort, ulBaudRate, eParity);
        break;
#endif
#if MB_ASCII_ENABLED > 0
    case MB_ASCII:
        dev->pvMBStartCur = vMBASCIIStart;
        dev->pvMBStopCur = vMBASCIIStop;
        dev->pvMBCloseCur = vMBASCIIClose;
        dev->peMBSendCur = eMBASCIISend;
        dev->peMBReceivedCur = eMBASCIIReceive;
        
        eStatus = eMBASCIIInit(dev, ucPort, ulBaudRate, eParity);
        break;
#endif
    default:
        eStatus = MB_EINVAL;
    }

    if(eStatus != MB_ENOERR){
        return eStatus;
    }
    
    if(!xMBSemBinaryInit(dev)){
        /* port dependent event module initalization failed. */
        eStatus = MB_EPORTERR;
    }
    else{
        dev->currentMode = eMode;
        dev->devstate = DEV_STATE_DISABLED;

        eStatus = __dev_add(dev);
    }
    
    return eStatus;
}
#endif

#if MB_TCP_ENABLED > 0
mb_ErrorCode_t eMBTCPOpen(mb_Device_t *dev, uint16_t ucTCPPort)
{
    mb_ErrorCode_t eStatus = MB_ENOERR;

    if(( eStatus = eMBTCPInit( ucTCPPort ) ) != MB_ENOERR){
         dev->devstate = DEV_STATE_DISABLED;
    }
    else if(!xMBSemBinaryInit(dev)){
        /* Port dependent event module initalization failed. */
        eStatus = MB_EPORTERR;
    }
    else{ 
        dev->pvMBStartCur = vMBTCPStart;
        dev->pvMBStopCur = vMBTCPStop;
        dev->pvMBCloseCur = vMBTCPClose;
        dev->peMBSendCur = eMBTCPReceive;
        dev->peMBReceivedCur = eMBTCPSend;
                
        dev->slaveid = MB_TCP_PSEUDO_ADDRESS;
        dev->port = ucTCPPort;
        dev->currentMode = MB_TCP;
        dev->devstate = DEV_STATE_DISABLED;

        eStatus = __dev_add(dev);
    }
    
    return eStatus;
}
#endif

//__align(2)  
//static uint8_t regbuf[REG_COILS_SIZE / 8 + REG_DISCRETE_SIZE / 8 + REG_INPUT_NREGS * 2 + REG_HOLDING_NREGS * 2];

mb_ErrorCode_t eMBRegCreate(mb_Device_t *dev,
                                uint8_t *regbuf,
                                uint16_t reg_holding_addr_start,
                                uint16_t reg_holding_num,
                                uint16_t reg_input_addr_start,
                                uint16_t reg_input_num,
                                uint16_t reg_coils_addr_start,
                                uint16_t reg_coils_num,
                                uint16_t reg_discrete_addr_start,
                                uint16_t reg_discrete_num)
{
    uint32_t lens;
    mb_Reg_t *regs;

    if(dev == NULL || regbuf == NULL)
        return MB_EINVAL;

    regs = (mb_Reg_t *)&dev->regs;

    regs->reg_holding_addr_start = reg_holding_addr_start;
    regs->reg_holding_num = reg_holding_num;
    regs->reg_input_addr_start = reg_input_addr_start;    
    regs->reg_input_num = reg_input_num;
        
    regs->reg_coils_addr_start = reg_coils_addr_start;
    regs->reg_coils_num = reg_coils_num;
    regs->reg_discrete_addr_start = reg_discrete_addr_start;
    regs->reg_discrete_num = reg_discrete_num;

    regs->pReghold = (uint16_t *)&regbuf[0];
    
    lens = reg_holding_num * sizeof(uint16_t);
    regs->pReginput = (uint16_t *)&regbuf[lens];
    
    lens +=  reg_input_num * sizeof(uint16_t);
    regs->pRegCoil = &regbuf[lens];
    
    lens += (reg_coils_num >> 3) + (((reg_coils_num & 0x07) > 0) ? 1 : 0);
    regs->pRegDisc = &regbuf[lens];
    
    lens += (reg_discrete_num >> 3) + (((reg_discrete_num & 0x07) > 0) ? 1 : 0);
    
    return MB_ENOERR;
}

mb_ErrorCode_t eMBStart(mb_Device_t *dev)
{
    if( dev->devstate == DEV_STATE_NOT_INITIALIZED )
        return MB_EILLSTATE;

    if( dev->devstate == DEV_STATE_DISABLED ){
        /* Activate the protocol stack. */
        dev->pvMBStartCur(dev);
        dev->devstate = DEV_STATE_ENABLED;
    }

    return MB_ENOERR;
}

mb_ErrorCode_t eMBStop(mb_Device_t *dev)
{
    if( dev->devstate == DEV_STATE_NOT_INITIALIZED )
        return MB_EILLSTATE;

    if( dev->devstate == DEV_STATE_ENABLED ){
        dev->pvMBStopCur(dev);
        dev->devstate = DEV_STATE_DISABLED;
    }

    return MB_ENOERR;
}

mb_ErrorCode_t eMBClose(mb_Device_t *dev)
{
    // must be stop first then it can close
    if( dev->devstate == DEV_STATE_DISABLED ){
        if( dev->pvMBCloseCur != NULL ){
            dev->pvMBCloseCur(dev);
        }

        return MB_ENOERR;
    }
    
    return MB_EILLSTATE;
}

void vMBPoll(void)
{
    mb_Device_t *curdev = mb_dev_head;    

     while(curdev){
        eMBADUFramehandle(curdev);
        curdev = curdev->next;
    }
}
mb_ErrorCode_t eMBRegisterCB( uint8_t ucFunctionCode, pxMBFunctionHandler pxHandler )
{
    int i;
    mb_ErrorCode_t eStatus;

    if((ucFunctionCode >= MB_FUNC_MIN) && (ucFunctionCode <= MB_FUNC_MAX)){
        ENTER_CRITICAL_SECTION(  );
        if( pxHandler != NULL ){
            
            for( i = 0; i < MB_FUNC_HANDLERS_MAX; i++ ){
                if( ( xFuncHandlers[i].pxHandler == NULL ) 
                    || ( xFuncHandlers[i].pxHandler == pxHandler ) ){
                    xFuncHandlers[i].ucFunctionCode = ucFunctionCode;
                    xFuncHandlers[i].pxHandler = pxHandler;
                    break;
                }
            }
            eStatus = ( i != MB_FUNC_HANDLERS_MAX ) ? MB_ENOERR : MB_ENORES;
        }
        else{
            for( i = 0; i < MB_FUNC_HANDLERS_MAX; i++ ){
                
                if( xFuncHandlers[i].ucFunctionCode == ucFunctionCode ){
                    xFuncHandlers[i].ucFunctionCode = 0;
                    xFuncHandlers[i].pxHandler = NULL;
                    break;
                }
            }
            /* Remove can't fail. */
            eStatus = MB_ENOERR;
        }
        EXIT_CRITICAL_SECTION(  );
    }
    else{
        eStatus = MB_EINVAL;
    }
    
    return eStatus;
}

static mb_ErrorCode_t eMBADUFramehandle(mb_Device_t *dev)
{
    uint8_t *pPduFrame; // pdu fram
    uint8_t ucRcvAddress;
    uint8_t ucFunctionCode;
    uint16_t usLength;
    eMBException_t eException;

    int i;
    mb_ErrorCode_t eStatus = MB_ENOERR;

    /* Check if the protocol stack is ready. */
    if( dev->devstate != DEV_STATE_ENABLED ){
        return MB_EILLSTATE;
    }

    /* Check if there is a event available. If not return control to caller.
     * Otherwise we will handle the event. */
    if(xMBSemTake(dev)){
            
        /* parser a adu fram */
        eStatus = dev->peMBReceivedCur(dev, &ucRcvAddress, &pPduFrame, &usLength );
        if( eStatus != MB_ENOERR )
            return eStatus;
        
        /* Check if the frame is for us. If not ignore the frame. */
        if((ucRcvAddress == dev->slaveid) || (ucRcvAddress == MB_ADDRESS_BROADCAST) ){
            ucFunctionCode = pPduFrame[MB_PDU_FUNC_OFF];
            eException = MB_EX_ILLEGAL_FUNCTION;
            for( i = 0; i < MB_FUNC_HANDLERS_MAX; i++ ){
                /* No more function handlers registered. Abort. */
                if( xFuncHandlers[i].ucFunctionCode == 0 ){
                    break;
                }
                else if(xFuncHandlers[i].ucFunctionCode == ucFunctionCode){
                    eException = xFuncHandlers[i].pxHandler(&dev->regs,pPduFrame, &usLength);
                    break;
                }
            }
            
            /* If the request was not sent to the broadcast address we return a reply. */
            if(ucRcvAddress == MB_ADDRESS_BROADCAST)
                return MB_ENOERR;

            /* send a reply */
            if(eException != MB_EX_NONE){
                /* An exception occured. Build an error frame. */
                usLength = 0;
                pPduFrame[usLength++] = (uint8_t)(ucFunctionCode | MB_FUNC_ERROR);
                pPduFrame[usLength++] = eException;
            }
            
            if((dev->currentMode == MB_ASCII) && MB_ASCII_TIMEOUT_WAIT_BEFORE_SEND_MS){
                vMBPortTimersDelay(dev->port, MB_ASCII_TIMEOUT_WAIT_BEFORE_SEND_MS );
            }        
            
            (void)dev->peMBSendCur(dev,dev->slaveid, pPduFrame, usLength);
        }
    }
    
    return MB_ENOERR;
}

static mb_ErrorCode_t __dev_add(mb_Device_t *dev)
{
    
    if(__dev_search(dev->port))
        return MB_EDEVEXIST;

    if(mb_dev_head == NULL){
        dev->next = NULL;
        mb_dev_head = dev;
    }
    else{
        dev->next = mb_dev_head;
        mb_dev_head = dev;
    }
    
    return MB_ENOERR;
}

/* RTU 和 ASCII 的硬件口是唯一的，不可重复
 * TCP service 端口也是唯一的
 */
static mb_Device_t *__dev_search(uint8_t port)
{
    mb_Device_t *srh = NULL;
    
    if(mb_dev_head == NULL)
        return NULL;

    srh = mb_dev_head;

    while(srh)
    {
        if(srh->port == port)
            return srh;

        srh = srh->next;
    }

    return NULL;
}

