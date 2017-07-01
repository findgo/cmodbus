#include "port.h"
#include "mb.h"
#if MB_RTU_ENABLED > 0
#include "mbrtu.h"
#endif
#if MB_ASCII_ENABLED > 0
#include "mbascii.h"
#endif
#if MB_TCP_ENABLED > 0
#include "mbtcp.h"
#endif
#include "mbfunc.h"
#include "mbutils.h"

#if (MB_RTU_ENABLED > 0 || MB_ASCII_ENABLED > 0) && MB_SLAVE_ENABLED > 0

static mb_ErrorCode_t __eMbsADUFramehandle(mbs_Device_t *dev);

#if MB_DYNAMIC_MEMORY_ALLOC_ENABLED == 0

#if MBS_SUPPORT_MULTIPLE_NUMBER > 1
static uint8_t mbs_devmask = 0;
static mbs_Device_t mbs_devTal[MBS_SUPPORT_MULTIPLE_NUMBER];
#else 
static mbs_Device_t mbs_devTal;
#endif

#else /* use dynamic memory */
static void __dev_add(mbs_Device_t *dev);
static mbs_Device_t *__dev_search(uint8_t port);

static mbs_Device_t *mbs_dev_head = NULL;

#endif

mbs_Device_t *xMbsNew(mb_Mode_t eMode, uint8_t ucSlaveAddress, 
                        uint8_t ucPort, uint32_t ulBaudRate, mb_Parity_t eParity )
{
    mbs_Device_t *dev = NULL;
    mb_ErrorCode_t eStatus;
#if MBS_SUPPORT_MULTIPLE_NUMBER > 1
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
    
    dev = mb_malloc(sizeof(mbs_Device_t));
    if(dev == NULL)
        return NULL;
#else
#if MBS_SUPPORT_MULTIPLE_NUMBER > 1
    /* check port exist ?*/
    idx = 0;
    mask = (uint8_t)1 << idx; 
    while(mbs_devmask & mask)
    {
        if(mbs_devTal[idx].port == ucPort){
                return NULL; /* exist */
        }
        idx++;
        mask <<= 1;
    }
    /* check range */
    if(idx >= MBS_SUPPORT_MULTIPLE_NUMBER)
        return NULL;
    /* can find it,alloc a dev object*/
    mbs_devmask |= (uint8_t)1 << idx; /* mark it */
    dev = (mbs_Device_t *)&mbs_devTal[idx];
#else
    dev = (mbs_Device_t *)&mbs_devTal;
#endif    
#endif
    memset(dev,0,sizeof(mbs_Device_t));    

    switch (eMode){
#if MB_RTU_ENABLED > 0
    case MB_RTU:
        dev->pvMBStartCur = vMbsRTUStart;
        dev->pvMBStopCur = vMbsRTUStop;
        dev->pvMBCloseCur = vMbsRTUClose;
        dev->peMBSendCur = eMbsRTUSend;
        dev->peMBReceivedCur = eMbsRTUReceive;

        eStatus = eMbsRTUInit(dev, ucPort, ulBaudRate, eParity);
        break;
#endif
#if MB_ASCII_ENABLED > 0
    case MB_ASCII:
        dev->pvMBStartCur = vMbsASCIIStart;
        dev->pvMBStopCur = vMbsASCIIStop;
        dev->pvMBCloseCur = vMbsASCIIClose;
        dev->peMBSendCur = eMbsASCIISend;
        dev->peMBReceivedCur = eMbsASCIIReceive;
        
        eStatus = eMbsASCIIInit(dev, ucPort, ulBaudRate, eParity);
        break;
#endif
    default:
        eStatus = MB_EINVAL;
    }

    if(eStatus != MB_ENOERR){
        
#if MB_DYNAMIC_MEMORY_ALLOC_ENABLED > 0
        mb_free(dev);
#else
#if MBS_SUPPORT_MULTIPLE_NUMBER > 1
        mbs_devmask &= ~((uint8_t)1 << idx); /* delete it */ 
        mbs_devTal[idx].devstate = DEV_STATE_NOT_INITIALIZED;
#else
        mbs_devTal.devstate = DEV_STATE_NOT_INITIALIZED;
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

void vMbsFree(uint8_t ucPort)
{
#if MB_DYNAMIC_MEMORY_ALLOC_ENABLED > 0
    mbs_Device_t *srh = NULL;
    mbs_Device_t *pre = NULL;
    
    srh = mbs_dev_head;
    pre = NULL;

    while(srh)
    {
        if(srh->port == ucPort)
            break;
        pre = srh;
        srh = srh->next;
    }

    if(srh){
        if(pre == NULL)
            mbs_dev_head = srh->next;
        else
            pre->next = srh->next;
        
        mb_free(srh);
    }
#else
#if MBS_SUPPORT_MULTIPLE_NUMBER > 1
    uint8_t idx,mask;
		/* check port exist ?*/
    idx = 0;
    mask = (uint8_t)1 << idx; 
    while(mbs_devmask & mask)
    {
        if(mbs_devTal[idx].port == ucPort){
            break;
        }
        idx++;
        mask <<= idx;
    }
    /* check range */
    if(idx < MBS_SUPPORT_MULTIPLE_NUMBER){
        /* can find it,alloc a dev object*/
        mbs_devmask &= ~((uint8_t)1 << idx); /* delete it */ 
        mbs_devTal[idx].devstate = DEV_STATE_NOT_INITIALIZED;
    }
#else
    mbs_devTal.devstate = DEV_STATE_NOT_INITIALIZED;
#endif    
#endif     
}

//__align(2)  
//static uint8_t regbuf[REG_COILS_SIZE / 8 + REG_DISCRETE_SIZE / 8 + REG_INPUT_NREGS * 2 + REG_HOLDING_NREGS * 2];
mb_ErrorCode_t eMbsRegAssign(mbs_Device_t *dev,
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

mb_ErrorCode_t eMbsStart(mbs_Device_t *dev)
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

mb_ErrorCode_t eMbsStop(mbs_Device_t *dev)
{
    if( dev->devstate == DEV_STATE_NOT_INITIALIZED )
        return MB_EILLSTATE;

    if( dev->devstate == DEV_STATE_ENABLED ){
        dev->pvMBStopCur(dev);
        dev->devstate = DEV_STATE_DISABLED;
    }

    return MB_ENOERR;
}

mb_ErrorCode_t eMbsClose(mbs_Device_t *dev)
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

void vMbsPoll(void)
{
#if MB_DYNAMIC_MEMORY_ALLOC_ENABLED > 0
    mbs_Device_t *srchdev;

    srchdev = mbs_dev_head;
    while(srchdev)
    {
        __eMbsADUFramehandle(srchdev);
        srchdev = srchdev->next;
    }
#else
#if MBS_SUPPORT_MULTIPLE_NUMBER > 1
    uint8_t idx;
    uint8_t mask;
    
    idx = 0;
    mask = (uint8_t)1 << idx; 
    while(mbs_devmask & mask)
	{
        __eMbsADUFramehandle(&mbs_devTal[idx]);
        idx++;
        mask <<=1;
    }
#else
    __eMbsADUFramehandle(&mbs_devTal);
#endif

#endif
}

static mb_ErrorCode_t __eMbsADUFramehandle(mbs_Device_t *dev)
{
    uint8_t *pPduFrame; // pdu fram
    uint8_t ucRcvAddress;
    uint8_t ucFunctionCode;
    uint16_t usLength;
    eMBException_t eException;

    pxMbsFunctionHandler handle;
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
            handle = xMbsSearchCB(ucFunctionCode);
            if(handle)
                eException = handle(&dev->regs,pPduFrame, &usLength);
            
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
            
            if((dev->currentMode == MB_ASCII) && MBS_ASCII_TIMEOUT_WAIT_BEFORE_SEND_MS){
                vMBPortTimersDelay(dev->port, MBS_ASCII_TIMEOUT_WAIT_BEFORE_SEND_MS );
            }        
            
            (void)dev->peMBSendCur(dev,dev->slaveaddr, pPduFrame, usLength);
        }
    }
    
    return MB_ENOERR;
}

#if MB_DYNAMIC_MEMORY_ALLOC_ENABLED > 0
static void __dev_add(mbs_Device_t *dev)
{
    if(mbs_dev_head == NULL){
        dev->next = NULL;
        mbs_dev_head = dev;
    }
    else{
        dev->next = mbs_dev_head;
        mbs_dev_head = dev;
    }
}

/* RTU 和 ASCII 的硬件口是唯一的，不可重复 */
static mbs_Device_t *__dev_search(uint8_t port)
{
    mbs_Device_t *srh = NULL;
    
    if(mbs_dev_head == NULL)
        return NULL;

    srh = mbs_dev_head;

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

