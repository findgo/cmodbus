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
#include "port.h"
#include "mbbuf.h"
#include "mbutils.h"
#include "mb.h"

/* TODO implement modbus master */
#if MB_MASTER_ENABLED > 0

typedef struct
{
    uint8_t ucFunctionCode;
    pxMBParseRspHandler pxHandler;
} xMBParseRspHandler;

static mb_ErrorCode_t eMBMasterhandle(mb_MasterDevice_t *dev,uint32_t timediff);
static mb_ErrorCode_t __masterReqreadylist_addtail(mb_MasterDevice_t *dev, mb_request_t *req);
/* peek ready list */
static mb_request_t *__masterReqreadylist_peek(mb_MasterDevice_t *dev);
static void __masterReqreadylist_removehead(mb_MasterDevice_t *dev);
static mb_ErrorCode_t __masterReqpendlist_add(mb_MasterDevice_t *dev, mb_request_t *req);
static void __masterReqpendlistScan(mb_MasterDevice_t *dev, uint32_t diff);
static mb_ErrorCode_t __masterdev_add(mb_MasterDevice_t *dev);
static mb_MasterDevice_t *__masterdev_search(uint8_t port);

static mb_MasterDevice_t *mb_masterdev_head = NULL;

static xMBParseRspHandler xParseRspHandlers[MB_FUNC_HANDLERS_MAX] = {
#if MB_PARSE_RSP_OTHER_REP_SLAVEID_ENABLED > 0
    {MB_FUNC_OTHER_REPORT_SLAVEID, NULL},
#endif
#if MB_PARSE_RSP_READ_HOLDING_ENABLED > 0
    {MB_FUNC_READ_HOLDING_REGISTER, eMBParseRspRdHoldingRegister},
#endif
#if MB_PARSE_RSP_WRITE_HOLDING_ENABLED > 0
    {MB_FUNC_WRITE_REGISTER, eMBParseRspWrHoldingRegister},
#endif
#if MB_PARSE_RSP_WRITE_MULTIPLE_HOLDING_ENABLED > 0
    {MB_FUNC_WRITE_MULTIPLE_REGISTERS, eMBParseRspWrMulHoldingRegister},
#endif
#if MB_PARSE_RSP_READWRITE_HOLDING_ENABLED > 0
    {MB_FUNC_READWRITE_MULTIPLE_REGISTERS, eMBParseRspRdWrMulHoldingRegister},
#endif
#if MB_PARSE_RSP_READ_INPUT_ENABLED > 0
    {MB_FUNC_READ_INPUT_REGISTER, eMBParseRdInputRegister},
#endif
#if MB_PARSE_RSP_READ_COILS_ENABLED > 0
    {MB_FUNC_READ_COILS, eMBParseRspRdCoils},
#endif
#if MB_PARSE_RSP_WRITE_COIL_ENABLED > 0
    {MB_FUNC_WRITE_SINGLE_COIL, eMBParseRspWrCoil},
#endif
#if MB_PARSE_RSP_WRITE_MULTIPLE_COILS_ENABLED > 0
    {MB_FUNC_WRITE_MULTIPLE_COILS, eMBParseRspWrMulCoils},
#endif
#if MB_PARSE_RSP_READ_DISCRETE_INPUTS_ENABLED > 0
    {MB_FUNC_READ_DISCRETE_INPUTS, eMBParseRspRdDiscreteInputs},
#endif
};

mb_MasterDevice_t *xMBBaseMasterDeviceNew(void)
{
    mb_MasterDevice_t *dev;
    
    dev = mb_malloc(sizeof(mb_MasterDevice_t));
    if(dev == NULL)
        return NULL;

    memset(dev,0,sizeof(mb_MasterDevice_t));
    
    return dev;
}

#if MB_RTU_ENABLED > 0 || MB_ASCII_ENABLED > 0
mb_MasterDevice_t *xMBMasterNew(mb_Mode_t eMode, uint8_t ucPort, uint32_t ulBaudRate, mb_Parity_t eParity)
{
    mb_ErrorCode_t eStatus = MB_ENOERR;
    mb_MasterDevice_t *dev;

    dev = mb_malloc(sizeof(mb_MasterDevice_t));
    if(dev == NULL)
        return NULL;

    switch (eMode){
#if MB_RTU_ENABLED > 0
    case MB_RTU:
        dev->pvMBStartCur = vMBMasterRTUStart;
        dev->pvMBStopCur = vMBMasterRTUStop;
        dev->pvMBCloseCur = vMBMasterRTUClose;
        dev->peMBSendCur = eMBMasterRTUSend;
        dev->peMBReceivedCur = eMBMasterRTUReceive;

        eStatus = eMBMasterRTUInit(dev, ucPort, ulBaudRate, eParity);
        break;
#endif
#if MB_ASCII_ENABLED > 0
    case MB_ASCII:
        dev->pvMBStartCur = vMBMasterASCIIStart;
        dev->pvMBStopCur = vMBMasterASCIIStop;
        dev->pvMBCloseCur = vMBMasterASCIIClose;
        dev->peMBSendCur = eMBMasterASCIISend;
        dev->peMBReceivedCur = eMBMasterASCIIReceive;
        
        eStatus = eMBMasterASCIIInit(dev, ucPort, ulBaudRate, eParity);
        break;
#endif
    default:
        eStatus = MB_EINVAL;
    }

    if(eStatus != MB_ENOERR){
        mb_free(dev);
        return NULL;
    }
    
    dev->port         = ucPort;
    dev->currentMode  = eMode;
    
    dev->devstate     = DEV_STATE_DISABLED;

    dev->nodehead     = NULL;

    dev->Reqreadyhead = NULL;
    dev->Reqreadytail = NULL;
    dev->Reqpendhead  = NULL;

    dev->Pollstate    = MASTER_IDLE;
    
    dev->retry        = MBM_DEFAULT_RETRYCNT;
    dev->retrycnt     = 0;
    
    dev->Replytimeout       = MBM_DEFAULT_REPLYTIMEOUT;
    dev->Replytimeoutcnt    = 0;
    dev->Delaypolltime      = MBM_DEFAULT_DELAYPOLLTIME;
    dev->Delaypolltimecnt   = 0;
    dev->Broadcastturntime  = MBM_DEFAULT_BROADTURNTIME;
    dev->Broadcastturntimecnt = 0;

    dev->next = NULL;
    
    eStatus = __masterdev_add(dev);
    
    if(eStatus != MB_ENOERR){
        mb_free(dev);
        return NULL;
    }
    
    return dev;
}
#endif

#if MB_TCP_ENABLED > 0
mb_ErrorCode_t eMBMasterTCPOpen(mb_MasterDevice_t *dev, uint16_t ucTCPPort)
{
    mb_ErrorCode_t eStatus = MB_ENOERR;

    if(( eStatus = eMBMasterTCPInit( ucTCPPort ) ) != MB_ENOERR){
         dev->devstate = DEV_STATE_DISABLED;
    }
    else{ 
        dev->pvMBStartCur = vMBMasterTCPStart;
        dev->pvMBStopCur = vMBMasterTCPStop;
        dev->pvMBCloseCur = vMBMasterTCPClose;
        dev->peMBSendCur = eMBMasterTCPReceive;
        dev->peMBReceivedCur = eMBMasterTCPSend;
                
        dev->port           = ucTCPPort;
        dev->currentMode    = MB_TCP;
        dev->devstate       = DEV_STATE_DISABLED;
        dev->retry          = MBM_DEFAULT_RETRYCNT;
        dev->retrycnt       = 0;
        dev->Replytimeout       = MBM_DEFAULT_REPLYTIMEOUT;
        dev->Replytimeoutcnt    = 0;
        dev->Delaypolltime      = MBM_DEFAULT_DELAYPOLLTIME;
        dev->Delaypolltimecnt   = 0;
        dev->Broadcastturntime  = MBM_DEFAULT_BROADTURNTIME;
        dev->Broadcastturntimecnt = 0;
        
        eStatus = __masterdev_add(dev);
    }
    
    return eStatus;
}
#endif

mb_ErrorCode_t eMBMasterDelete(uint8_t ucPort)
{
     mb_MasterDevice_t *srh = NULL;
     mb_MasterDevice_t *pre = NULL;
     
    if(mb_masterdev_head == NULL)
        return MB_ENOERR;

    srh = mb_masterdev_head;

    while(srh)
    {
        if(srh->port == ucPort)
            break;
        
        pre = srh;
        srh = srh->next;
    }

    if(srh){
        if(pre == NULL)
            mb_masterdev_head = srh->next;
        else
            pre->next = srh->next;
        
        mb_free(srh);
    }

    return MB_ENOERR;
}

mb_ErrorCode_t eMBMasterSetPara(mb_MasterDevice_t *dev, 
                                    uint8_t retry,uint32_t replytimeout,
                                    uint32_t delaypolltime, uint32_t broadcastturntime)
{
    if(dev){
        dev->retry = (retry > MBM_RETRYCNT_MAX) ? MBM_RETRYCNT_MAX : retry;
        
        if(replytimeout < MBM_REPLYTIMEOUT_MIN)
            dev->Replytimeout = MBM_REPLYTIMEOUT_MIN;
        else if(replytimeout > MBM_REPLYTIMEOUT_MAX)
            dev->Replytimeout = MBM_REPLYTIMEOUT_MAX;
        else
            dev->Replytimeout = replytimeout;

        if(delaypolltime < MBM_DELAYPOLLTIME_MIN)
            dev->Delaypolltime = MBM_DELAYPOLLTIME_MIN;
        else if(delaypolltime > MBM_DELAYPOLLTIME_MAX)
            dev->Delaypolltime = MBM_DELAYPOLLTIME_MAX;
        else
            dev->Delaypolltime = delaypolltime;

        if(broadcastturntime < MBM_DELAYPOLLTIME_MIN)
            dev->Broadcastturntime = MBM_BROADTURNTIME_MIN;
        else if(broadcastturntime > MBM_BROADTURNTIME_MAX)
            dev->Broadcastturntime = MBM_BROADTURNTIME_MAX;
        else
            dev->Broadcastturntime = broadcastturntime;

        return MB_ENOERR;
    }

    return MB_EINVAL;
}


/*在主机上创建一个从机节点       和 寄存器列表,并加入到主机节点列表*/
mb_slavenode_t *xMBMasterNodeNew(mb_MasterDevice_t *dev,
                                        uint8_t slaveaddr,
                                        uint16_t reg_holding_addr_start,
                                        uint16_t reg_holding_num,
                                        uint16_t reg_input_addr_start,
                                        uint16_t reg_input_num,
                                        uint16_t reg_coils_addr_start,
                                        uint16_t reg_coils_num,
                                        uint16_t reg_discrete_addr_start,
                                        uint16_t reg_discrete_num)
{
    mb_slavenode_t *node;
    
    if(dev == NULL)
        return NULL;

    /* check slave address valid */
    if(slaveaddr < MB_ADDRESS_MIN || slaveaddr > MB_ADDRESS_MAX)
        return NULL;
    
    /* check node on the host ?*/
    node = xMBMasterNodeSearch(dev,slaveaddr);
    if(node)
        return NULL;/* on the host */

    node = xMBMasterNodeCreate(slaveaddr, reg_holding_addr_start, reg_holding_num,reg_input_addr_start,reg_input_num,
                                   reg_coils_addr_start,reg_coils_num,reg_discrete_addr_start,reg_discrete_num);
    if(node){
        // add node to host node list
        if(dev->nodehead == NULL){
            dev->nodehead = node;
        }
        else{
            node->next = dev->nodehead;
            dev->nodehead = node;
        }        
    }

    return node;
}
/* 从主机删除一个从机节点 必需由NEW创建的才可以delete */
mb_ErrorCode_t eMBMasterNodedelete(mb_MasterDevice_t *dev, uint8_t slaveaddr)
{
    mb_slavenode_t *srchnode;
    mb_slavenode_t *prenode;
    
    if(dev == NULL)
        return MB_EINVAL;

    srchnode = dev->nodehead;
    prenode = NULL;
    /* search node on the host nodelist*/
    while(srchnode)
    {
        if(srchnode->slaveaddr == slaveaddr) // find it
            break;
        
        prenode = srchnode;
        srchnode = srchnode->next;
    }

    if(srchnode){
        //first remove from node list
        if(prenode == NULL)
            dev->nodehead = srchnode->next;
        else
            prenode->next = srchnode->next;
        
        vMBMasterNodeDestroy(srchnode);
    }
    
    return MB_ENOERR;
}
/* 创建一个独立节点和寄存器列表 */
mb_slavenode_t *xMBMasterNodeCreate(uint8_t slaveaddr,
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
    uint8_t *regbuf;
    mb_Reg_t *reg;
    mb_slavenode_t *node;

    /* check slave address valid */
    if(slaveaddr < MB_ADDRESS_MIN || slaveaddr > MB_ADDRESS_MAX)
        return NULL;
    
    node = mb_malloc(sizeof(mb_slavenode_t));
    if(node){
        lens = xMBRegBufSizeCal(reg_holding_num,reg_input_num,reg_coils_num,reg_discrete_num);

        regbuf = mb_malloc(lens);
        if(regbuf == NULL){
            mb_free(node);
            return NULL;
        }
        reg = (mb_Reg_t *)&node->regs;
        
        reg->reg_holding_addr_start = reg_holding_addr_start;
        reg->reg_holding_num = reg_holding_num;
        reg->reg_input_addr_start = reg_input_addr_start;    
        reg->reg_input_num = reg_input_num;
            
        reg->reg_coils_addr_start = reg_coils_addr_start;
        reg->reg_coils_num = reg_coils_num;
        reg->reg_discrete_addr_start = reg_discrete_addr_start;
        reg->reg_discrete_num = reg_discrete_num;

        reg->pReghold = (uint16_t *)&regbuf[0];
        
        lens = reg_holding_num * sizeof(uint16_t);
        reg->pReginput = (uint16_t *)&regbuf[lens];
        
        lens +=  reg_input_num * sizeof(uint16_t);
        reg->pRegCoil = (uint8_t *)&regbuf[lens];
        
        lens += (reg_coils_num >> 3) + (((reg_coils_num & 0x07) > 0) ? 1 : 0);
        reg->pRegDisc = (uint8_t *)&regbuf[lens];
        
        node->slaveaddr = slaveaddr;
        node->next = NULL;
    }

    return node;
}
/* 释放节点，释放由Create创建的节点 */
void vMBMasterNodeDestroy(mb_slavenode_t *node)
{
    if(node){
        if(node->regs.pReghold)
            mb_free(node->regs.pReghold);
        mb_free(node);
    }
}

/* 将节点加入到主机 */
mb_ErrorCode_t eMBMasterNodeadd(mb_MasterDevice_t *dev, mb_slavenode_t *node)
{
    mb_slavenode_t *srhnode;
    
    if(dev == NULL || node == NULL)
        return MB_EINVAL;

    srhnode = xMBMasterNodeSearch(dev,node->slaveaddr);
    if(srhnode)
        return MBM_ENODEEXIST;
    
    if(dev->nodehead == NULL){
        dev->nodehead = node;
    }
    else{
        node->next = dev->nodehead;
        dev->nodehead = node;
    }

    return MB_ENOERR;
}

mb_ErrorCode_t eMBMasterNoderemove(mb_MasterDevice_t *dev, mb_slavenode_t *node)
{
    mb_slavenode_t *srchnode;
    mb_slavenode_t *prenode;
    
    if(dev == NULL || node == NULL)
        return MB_EINVAL;

    srchnode = dev->nodehead;
    prenode = NULL;
    /* search node on the host nodelist*/
    while(srchnode)
    {
        if(srchnode->slaveaddr == node->slaveaddr) // find it
            break;
        
        prenode = srchnode;
        srchnode = srchnode->next;
    }

    if(srchnode){
        //first remove from node list
        if(prenode == NULL)
            dev->nodehead = srchnode->next;
        else
            prenode->next = srchnode->next;
    }
    
    return MB_ENOERR;
}

/* search node on the host list ? */
mb_slavenode_t *xMBMasterNodeSearch(mb_MasterDevice_t *dev,uint8_t slaveaddr)
{
    mb_slavenode_t *srh;

    if(dev == NULL)
        return NULL;

    srh = dev->nodehead;

    while(srh)
    {
        if(srh->slaveaddr == slaveaddr)
            return srh;

        srh = srh->next;
    }

    return NULL;
}


mb_ErrorCode_t eMBMasterStart(mb_MasterDevice_t *dev)
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

mb_ErrorCode_t eMBMasterStop(mb_MasterDevice_t *dev)
{
    if( dev->devstate == DEV_STATE_NOT_INITIALIZED )
        return MB_EILLSTATE;

    if( dev->devstate == DEV_STATE_ENABLED ){
        dev->pvMBStopCur(dev);
        dev->devstate = DEV_STATE_DISABLED;
    }

    return MB_ENOERR;
}

mb_ErrorCode_t eMBMasterClose(mb_MasterDevice_t *dev)
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

void vMBMasterPoll(void)
{
    static uint32_t HistimerCounter = 0;
    mb_MasterDevice_t *curdev;    
    uint32_t elapsedMSec = 0;

    elapsedMSec = (uint32_t)(xMBsys_now() - HistimerCounter);
    if(elapsedMSec)
        HistimerCounter = xMBsys_now();

    curdev = mb_masterdev_head;
     while(curdev){
        eMBMasterhandle(curdev,elapsedMSec);
        curdev = curdev->next;
    }
}

static mb_ErrorCode_t eMBMasterhandle(mb_MasterDevice_t *dev,uint32_t timediff)
{
    uint8_t *pRemainFrame; // remain fram
    uint8_t ucFunctionCode;
    uint16_t usLength;
    mb_ErrorCode_t status;
    eMBException_t eException;
    mb_header_t header;
    mb_request_t *req;

    int i;
    mb_ErrorCode_t eStatus = MB_ENOERR;

    /* Check if the protocol stack is ready. */
    if( dev->devstate != DEV_STATE_ENABLED ){
        return MB_EILLSTATE;
    }
      
    switch (dev->Pollstate){
    case MASTER_IDLE:
        dev->Delaypolltimecnt = 0;
        dev->Pollstate = MASTER_DELYPOLL;
        break;
    case MASTER_XMIT: 
        req = __masterReqreadylist_peek(dev);
        if(req && (dev->peMBSendCur(dev,req->padu,req->adulength) == MB_ENOERR)){
            dev->Pollstate = MASTER_XMITING;
        }
        else{ /* nothing want to send or send error, wait a moment to try*/
            dev->Pollstate = MASTER_DELYPOLL;
        }
        break;

    case MASTER_RSPEXCUTE:        
        req = __masterReqreadylist_peek(dev);
        if(req == NULL) { /* some err happen ,then no request in list*/
            dev->Pollstate = MASTER_DELYPOLL;
            break;
        }
    
        /* parser a adu fram */
        eStatus = dev->peMBReceivedCur(dev, &header, &ucFunctionCode, &pRemainFrame, &usLength);
        if( eStatus != MB_ENOERR ){
            dev->Pollstate = MASTER_DELYPOLL;
            break;
        }

        if((req->funcode != (ucFunctionCode & 0x7f)) || (req->slaveaddr != header.introute.slaveid)){
            /* not for us ,continue to wait response */
            dev->Pollstate = MASTER_WAITRSP;
            break;
        }
        
        /* funcoe and slaveid same, this frame for us and then excute it*/
        if(ucFunctionCode & 0x80){ // 异常码
            eException = (eMBException_t)pRemainFrame[0]; //异常码
            __masterReqreadylist_removehead(dev); // remove from ready list
            if((req->slaveaddr == MB_ADDRESS_BROADCAST) || (req->scanrate == 0))// only once
                vMB_ReqBufDelete(req); 
            else{
                __masterReqpendlist_add(dev, req);// move to pend list
            }                
        }
        else{
            status = MBM_EINFUNCTION;
            for( i = 0; i < MB_PARSE_RSP_HANDLERS_MAX; i++ ){
                /* No more function handlers registered. Abort. */
                if( xParseRspHandlers[i].ucFunctionCode == 0 ){
                    break;
                }
                else if(xParseRspHandlers[i].ucFunctionCode == ucFunctionCode){
                    status = xParseRspHandlers[i].pxHandler(&req->node->regs, 
                                                            req->regaddr, req->regcnt, 
                                                            pRemainFrame,usLength);
                    break;
                }                
            }

            if(status == MBM_EINFUNCTION){ // 无此功能码，// remove from ready list
                __masterReqreadylist_removehead(dev); // remove from ready list
                vMB_ReqBufDelete(req); // delete request
            }
            else {
                //if(status == MB_ENOERR){
                    __masterReqreadylist_removehead(dev); // remove from ready list
                    if((req->slaveaddr == MB_ADDRESS_BROADCAST) || (req->scanrate == 0))// only once
                        vMB_ReqBufDelete(req); 
                    else{
                        __masterReqpendlist_add(dev, req);// move to pend list
                    }                
            }         
        }
        dev->Pollstate = MASTER_DELYPOLL;             
        break;
    case MASTER_RSPTIMEOUT:
        req = __masterReqreadylist_peek(dev);
        if(req == NULL) { /* some err happen ,then no request in list*/
            dev->Pollstate = MASTER_DELYPOLL;
        }
        else{
            __masterReqreadylist_removehead(dev);
            if((req->slaveaddr == MB_ADDRESS_BROADCAST) || (req->scanrate == 0))// only once
                vMB_ReqBufDelete(req);
            else{// move to pend list
                __masterReqpendlist_add(dev, req);
            }
            dev->Pollstate = MASTER_XMIT;
        }
        break;
    case MASTER_BROADCASTTURN:
    case MASTER_DELYPOLL:
    case MASTER_XMITING:
        break;
    case MASTER_WAITRSP:
        req = __masterReqreadylist_peek(dev);
        if(req == NULL) { /* some err happen ,then no request in list*/
            dev->Pollstate = MASTER_DELYPOLL;
        }
        else if(req->slaveaddr == MB_ADDRESS_BROADCAST){           
            __masterReqreadylist_removehead(dev);
            vMB_ReqBufDelete(req);
            dev->Pollstate = MASTER_BROADCASTTURN;
        }
        else{
            /* keep wait for responese */
        }
        break;        
    default:
        dev->Pollstate = MASTER_IDLE;
    }
    
    if(timediff){
        switch (dev->Pollstate){
        case MASTER_BROADCASTTURN:
            /* 广播转换延迟时间 ,发出广播后给节点处理的时间*/
            dev->Broadcastturntimecnt += timediff;
            if(dev->Broadcastturntimecnt >= dev->Broadcastturntime){
                dev->Broadcastturntimecnt = 0;                
                dev->Pollstate = MASTER_XMIT;
            }
            break;
        case MASTER_DELYPOLL:
            /* 两个请求之间的延迟时间 */
            dev->Delaypolltimecnt += timediff;
            if(dev->Delaypolltimecnt >= dev->Delaypolltime){
                dev->Delaypolltimecnt = 0;                
                dev->Pollstate = MASTER_XMIT;
            }
            break;
        case MASTER_WAITRSP:
            dev->Replytimeoutcnt += timediff;
            if(dev->Replytimeoutcnt >= dev->Replytimeout){
                dev->Replytimeoutcnt = 0;
                dev->Pollstate = MASTER_RSPTIMEOUT;
            }
            break;
        default:
            break;
        }

        __masterReqpendlistScan(dev,timediff);// scan pend list 
    }
    
    return MB_ENOERR;
}


mb_ErrorCode_t eMBMaster_Reqsend(mb_MasterDevice_t *dev, mb_request_t *req)
{   
    uint16_t crc_lrc;
    
    if(dev->currentMode == MB_RTU){
#if MB_RTU_ENABLED > 0
        crc_lrc = prvxMBCRC16(req->padu,req->adulength);
        req->padu[req->adulength++] = crc_lrc & 0xff;
        req->padu[req->adulength++] = (crc_lrc >> 8) & 0xff;
#endif
    }else if(dev->currentMode == MB_ASCII){
#if MB_ASCII_ENABLED > 0
        crc_lrc = prvxMBLRC(req->padu,req->adulength);
        req->padu[req->adulength++] = crc_lrc & 0xff;
#endif
    }else{
        /* tcp no check sum */
    }
    
    if(!req->scanrate) // if zero add ready list immediately but only once
        __masterReqreadylist_addtail(dev,req);
    else
        __masterReqpendlist_add(dev,req);

    return MB_ENOERR;
}


/*## 向主机就绪列表尾部增加一个请求 这是个fifo的队列*/
static mb_ErrorCode_t __masterReqreadylist_addtail(mb_MasterDevice_t *dev, mb_request_t *req)
{
    req->next = NULL; /* make sure next is NULL , may be link previous list*/
    if(dev->Reqreadyhead == NULL){
        dev->Reqreadyhead = req;
    }
    else{
        dev->Reqreadytail->next = req;
    }
    dev->Reqreadytail = req;
    
    return MB_ENOERR;
}

/* peek ready list */
static mb_request_t *__masterReqreadylist_peek(mb_MasterDevice_t *dev)
{
    return dev->Reqreadyhead;
}

static void __masterReqreadylist_removehead(mb_MasterDevice_t *dev)
{
    if(dev == NULL || dev->Reqreadyhead == NULL) /* nothing to remove */ 
        return;
    
    dev->Reqreadyhead = dev->Reqreadyhead->next;
    if(dev->Reqreadyhead == NULL) /* it is reach tail and noting in the list */
        dev->Reqreadytail = NULL;
}


/*## 向主机挂起列表增加一个请求 */
static mb_ErrorCode_t __masterReqpendlist_add(mb_MasterDevice_t *dev, mb_request_t *req)
{
    req->scancnt = 0;  // clear sacn count
    if(dev->Reqpendhead == NULL){
        req->next = NULL; // make sure next is NULL
    }
    else{
        req->next = dev->Reqpendhead;
    }
    
    dev->Reqpendhead = req;
    
    return MB_ENOERR;
}

/*##  */
static void __masterReqpendlistScan(mb_MasterDevice_t *dev, uint32_t diff)
{
    mb_request_t *prevReq;
    mb_request_t *srchReq;
    mb_request_t *tempReq;
    
    srchReq = dev->Reqpendhead;
    prevReq = NULL;
    while(srchReq)
    {
        srchReq->scancnt += diff;
        if(srchReq->scancnt >= srchReq->scanrate){// scan timeout move to ready list
            
            //first remove from pend list
            if(prevReq == NULL)
                dev->Reqpendhead = srchReq->next;
            else
                prevReq->next = srchReq->next;

            tempReq = srchReq;  /* set up to add ready list later */
            srchReq = srchReq->next; /* get next */
            
            // then add to read list
            __masterReqreadylist_addtail(dev,tempReq);
        }
        else{
            /* get next */
            prevReq = srchReq;
            srchReq = srchReq->next;
        }
    }
}


/* ok */
static mb_ErrorCode_t __masterdev_add(mb_MasterDevice_t *dev)
{
    
    if(__masterdev_search(dev->port))
        return MB_EDEVEXIST;

    if(mb_masterdev_head == NULL){
        dev->next = NULL;
        mb_masterdev_head = dev;
    }
    else{
        dev->next = mb_masterdev_head;
        mb_masterdev_head = dev;
    }
    
    return MB_ENOERR;
}

/*ok 
 * RTU 和 ASCII 的硬件口是唯一的，不可重复
 * TCP service 端口也是唯一的
 */
static mb_MasterDevice_t *__masterdev_search(uint8_t port)
{
    mb_MasterDevice_t *srh = NULL;
    
    if(mb_masterdev_head == NULL)
        return NULL;

    srh = mb_masterdev_head;

    while(srh)
    {
        if(srh->port == port)
            return srh;

        srh = srh->next;
    }

    return NULL;
}


#endif

