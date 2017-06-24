
#include "port.h"
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

#if MB_SLAVE_ENABLED > 0

typedef struct
{
    uint8_t ucFunctionCode;
    pxMBFunctionHandler pxHandler;
} xMBFunctionHandler;

static mb_ErrorCode_t __eMBADUFramehandle(mb_Device_t *dev);

/* An array of Modbus functions handlers which associates Modbus function
 * codes with implementing functions.
 */
static xMBFunctionHandler xFuncHandlers[MB_FUNC_HANDLERS_MAX] = {
#if MB_FUNC_OTHER_REP_SLAVEID_ENABLED > 0
    {MB_FUNC_OTHER_REPORT_SLAVEID, eMBFuncReportSlaveID},
#endif
#if MB_FUNC_READ_HOLDING_ENABLED > 0
    {MB_FUNC_READ_HOLDING_REGISTER, eMBFuncRdHoldingRegister},
#endif
#if MB_FUNC_WRITE_HOLDING_ENABLED > 0
    {MB_FUNC_WRITE_REGISTER, eMBFuncWrHoldingRegister},
#endif
#if MB_FUNC_WRITE_MULTIPLE_HOLDING_ENABLED > 0
    {MB_FUNC_WRITE_MULTIPLE_REGISTERS, eMBFuncWrMulHoldingRegister},
#endif
#if MB_FUNC_READWRITE_HOLDING_ENABLED > 0
    {MB_FUNC_READWRITE_MULTIPLE_REGISTERS, eMBFuncRdWrMulHoldingRegister},
#endif
#if MB_FUNC_READ_INPUT_ENABLED > 0
    {MB_FUNC_READ_INPUT_REGISTER, eMBFuncRdInputRegister},
#endif
#if MB_FUNC_READ_COILS_ENABLED > 0
    {MB_FUNC_READ_COILS, eMBFuncRdCoils},
#endif
#if MB_FUNC_WRITE_COIL_ENABLED > 0
    {MB_FUNC_WRITE_SINGLE_COIL, eMBFuncWrCoil},
#endif
#if MB_FUNC_WRITE_MULTIPLE_COILS_ENABLED > 0
    {MB_FUNC_WRITE_MULTIPLE_COILS, eMBFuncWrMulCoils},
#endif
#if MB_FUNC_READ_DISCRETE_INPUTS_ENABLED > 0
    {MB_FUNC_READ_DISCRETE_INPUTS, eMBFuncRdDiscreteInputs},
#endif
};
#if MB_DYNAMIC_MEMORY_ALLOC_ENABLED == 0
#if MB_SUPPORT_MULTIPLE_NUMBER > 1
static uint8_t mb_devmask = 0;
static mb_Device_t mb_devTal[MB_SUPPORT_MULTIPLE_NUMBER];
#else 
static mb_Device_t mb_devTal;
#endif

#else /* use dynamic memory */
static void __dev_add(mb_Device_t *dev);
static mb_Device_t *__dev_search(uint8_t port);

static mb_Device_t *mb_dev_head = NULL;
uint8_t *xMBRegBufNew(uint32_t size)
{
    uint8_t *pregbuf;
    
    pregbuf = mb_malloc(size);
    if(pregbuf == NULL)
        return NULL;
    
    memset(pregbuf,0,sizeof(mb_Device_t));
    
    return pregbuf;    
}
#endif

#if MB_RTU_ENABLED > 0 || MB_ASCII_ENABLED > 0
mb_Device_t *xMBNew(mb_Mode_t eMode, uint8_t ucSlaveAddress, 
                        uint8_t ucPort, uint32_t ulBaudRate, mb_Parity_t eParity )
{
    mb_Device_t *dev = NULL;
    mb_ErrorCode_t eStatus;
#if MB_SUPPORT_MULTIPLE_NUMBER > 1
    uint8_t mask;
    uint8_t idx;
#endif

    /* check preconditions */
    if((ucSlaveAddress == MB_ADDRESS_BROADCAST) \
        || ( ucSlaveAddress < MB_ADDRESS_MIN ) \
        || ( ucSlaveAddress > MB_ADDRESS_MAX )){
        
        return NULL;
    }
#if MB_DYNAMIC_MEMORY_ALLOC_ENABLED > 0
    dev = __dev_search(ucPort);
    if(dev) /* exist ? */
        return NULL; 
    
    dev = mb_malloc(sizeof(mb_Device_t));
    if(dev == NULL)
        return NULL;
#else
#if MB_SUPPORT_MULTIPLE_NUMBER > 1
    /* check port exist ?*/
    idx = 0;
    mask = (uint8_t)1 << idx; 
    while(mb_devmask & mask)
    {
        if(mb_devTal[idx].port == ucPort){
                return NULL; /* exist */
        }
        idx++;
        mask <<= 1;
    }
    /* check range */
    if(idx >= MB_SUPPORT_MULTIPLE_NUMBER)
        return NULL;
    /* can find it,alloc a dev object*/
    mb_devmask |= (uint8_t)1 << idx; /* mark it */
    dev = (mb_Device_t *)&mb_devTal[idx];
#else
    dev = (mb_Device_t *)&mb_devTal;
#endif    
#endif
    memset(dev,0,sizeof(mb_Device_t));    

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
        
#if MB_DYNAMIC_MEMORY_ALLOC_ENABLED > 0
        mb_free(dev);
#else
#if MB_SUPPORT_MULTIPLE_NUMBER > 1
        mb_devmask &= ~((uint8_t)1 << idx); /* delete it */ 
        mb_devTal[idx].devstate = DEV_STATE_NOT_INITIALIZED;
#else
        mb_devTal.devstate = DEV_STATE_NOT_INITIALIZED;
#endif
#endif
        return NULL;
    }
    
    /* set slave address */
    dev->port = ucPort;
    dev->slaveaddr = ucSlaveAddress;
    dev->currentMode = eMode;
    dev->devstate = DEV_STATE_DISABLED;    
    dev->xEventInFlag = false;
#if MB_DYNAMIC_MEMORY_ALLOC_ENABLED > 0
    __dev_add(dev);
#endif

    return dev;
}
#endif

#if MB_TCP_ENABLED > 0
mb_ErrorCode_t eMBTCPNew(mb_Device_t *dev, uint16_t ucTCPPort)
{
    mb_ErrorCode_t eStatus = MB_ENOERR;

    if(( eStatus = eMBTCPInit( ucTCPPort ) ) != MB_ENOERR){
         dev->devstate = DEV_STATE_DISABLED;
    }
    else{ 
        dev->xEventInFlag = false;
        dev->pvMBStartCur = vMBTCPStart;
        dev->pvMBStopCur = vMBTCPStop;
        dev->pvMBCloseCur = vMBTCPClose;
        dev->peMBSendCur = eMBTCPReceive;
        dev->peMBReceivedCur = eMBTCPSend;
                
        dev->slaveaddr = MB_TCP_PSEUDO_ADDRESS;
        dev->port = ucTCPPort;
        dev->currentMode = MB_TCP;
        dev->devstate = DEV_STATE_DISABLED;

        __dev_add(dev);
    }
    
    return eStatus;
}
#endif
mb_ErrorCode_t eMBDelete(uint8_t ucPort)
{
#if MB_DYNAMIC_MEMORY_ALLOC_ENABLED > 0
    mb_Device_t *srh = NULL;
    mb_Device_t *pre = NULL;
    
    srh = mb_dev_head;
    pre = NULL;

    while(srh)
    {
        if(srh->port == ucPort)
            break;
        pre = srh;
        srh = srh->next;
    }

    if(srh){
        mb_free(srh);
    }
    
    return MB_ENOERR;
#else
#if MB_SUPPORT_MULTIPLE_NUMBER > 1
    uint8_t idx,mask;
		/* check port exist ?*/
    idx = 0;
    mask = (uint8_t)1 << idx; 
    while(mb_devmask & mask)
    {
        if(mb_devTal[idx].port == ucPort){
            break;
        }
        idx++;
        mask <<= idx;
    }
    /* check range */
    if(idx < MB_SUPPORT_MULTIPLE_NUMBER){
        /* can find it,alloc a dev object*/
        mb_devmask &= ~((uint8_t)1 << idx); /* delete it */ 
        mb_devTal[idx].devstate = DEV_STATE_NOT_INITIALIZED;
    }
#else
    mb_devTal.devstate = DEV_STATE_NOT_INITIALIZED;
#endif    
#endif    
}

//__align(2)  
//static uint8_t regbuf[REG_COILS_SIZE / 8 + REG_DISCRETE_SIZE / 8 + REG_INPUT_NREGS * 2 + REG_HOLDING_NREGS * 2];
mb_ErrorCode_t eMBRegAssign(mb_Device_t *dev,
                                uint8_t *regbuf,
                                uint32_t regbufsize, 
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

    if(regbufsize < xMBRegBufSizeCal(reg_holding_num, reg_input_num, reg_coils_num, reg_discrete_num))
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
#if MB_DYNAMIC_MEMORY_ALLOC_ENABLED > 0
    mb_Device_t *srchdev;

    srchdev = mb_dev_head;
    while(srchdev)
    {
        __eMBADUFramehandle(srchdev);
        srchdev = srchdev->next;
    }
#else
#if MB_SUPPORT_MULTIPLE_NUMBER > 1
    uint8_t idx;
    uint8_t mask;
    
    idx = 0;
    mask = (uint8_t)1 << idx; 
    while(mb_devmask & mask)
	{
        __eMBADUFramehandle(&mb_devTal[idx]);
        idx++;
        mask <<=1;
    }
#else
    __eMBADUFramehandle(&mb_devTal);
#endif

#endif
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

static mb_ErrorCode_t __eMBADUFramehandle(mb_Device_t *dev)
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
    if(dev->xEventInFlag){
        dev->xEventInFlag = false;
        /* parser a adu fram */
        eStatus = dev->peMBReceivedCur(dev, &ucRcvAddress, &pPduFrame, &usLength );
        if( eStatus != MB_ENOERR )
            return eStatus;
        
        /* Check if the frame is for us. If not ignore the frame. */
        if((ucRcvAddress == dev->slaveaddr) || (ucRcvAddress == MB_ADDRESS_BROADCAST) ){
            ucFunctionCode = pPduFrame[MB_PDU_FUNCODE_OFF];
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
            
            (void)dev->peMBSendCur(dev,dev->slaveaddr, pPduFrame, usLength);
        }
    }
    
    return MB_ENOERR;
}

#if MB_DYNAMIC_MEMORY_ALLOC_ENABLED > 0
static void __dev_add(mb_Device_t *dev)
{
    if(mb_dev_head == NULL){
        dev->next = NULL;
        mb_dev_head = dev;
    }
    else{
        dev->next = mb_dev_head;
        mb_dev_head = dev;
    }
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
#endif

#endif

