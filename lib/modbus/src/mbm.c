#include "mbfunc.h"
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
#include "mbutils.h"

/* TODO implement modbus master */
#if (MB_RTU_ENABLED > 0 || MB_ASCII_ENABLED > 0) && MB_MASTER_ENABLED > 0
#include "mbmem.h"
#include "mbmbuf.h"

static MbErrorCode_t __MbmHandle(MbmDev_t *dev,uint32_t timediff);
static MbErrorCode_t __masterReqreadylist_addtail(MbmDev_t *dev, MbmReq_t *req);
/* peek ready list */
static MbmReq_t *__masterReqreadylist_peek(MbmDev_t *dev);
static void __masterReqreadylist_removehead(MbmDev_t *dev);
static MbErrorCode_t __masterReqpendlist_add(MbmDev_t *dev, MbmReq_t *req);
static void __masterReqpendlistScan(MbmDev_t *dev, uint32_t diff);
static MbErrorCode_t __masterdev_add(MbmDev_t *dev);
static MbmDev_t *__masterdev_search(uint8_t port);

static MbmDev_t *mbm_dev_head = NULL;

MbmDev_t *MbmNew(MbMode_t eMode, uint8_t ucPort, uint32_t ulBaudRate, MbParity_t eParity)
{
    MbErrorCode_t eStatus = MB_ENOERR;
    MbmDev_t *dev;

    dev = (MbmDev_t *)mb_malloc(sizeof(MbmDev_t));
    if(dev == NULL)
        return NULL;
    
    memset(dev, 0, sizeof(MbmDev_t));    
    
    switch (eMode){
#if MB_RTU_ENABLED > 0
    case MB_RTU:
        dev->pMbStartCur = MbmRTUStart;
        dev->pMbStopCur = MbmRTUStop;
        dev->pMbCloseCur = MbmRTUClose;
        dev->pMbSendCur = MbmRTUSend;
        dev->pMbReceivedCur = MbmRTUReceive;

        eStatus = MbmRTUInit(dev, ucPort, ulBaudRate, eParity);
        break;
#endif

#if MB_ASCII_ENABLED > 0
    case MB_ASCII:
        dev->pMbStartCur = MbmASCIIStart;
        dev->pMbStopCur = MbmASCIIStop;
        dev->pMbCloseCur = MbmASCIIClose;
        dev->pMbSendCur = MbmASCIISend;
        dev->pMbReceivedCur = MbmASCIIReceive;
        
        eStatus = MbmASCIIInit(dev, ucPort, ulBaudRate, eParity);
        break;
#endif

    default:
        eStatus = MB_EINVAL;
        break;
        
    }

    if(eStatus != MB_ENOERR){
        mb_free(dev);
        return NULL;
    }

    // init parameter
    dev->port         = ucPort;
    dev->currentMode  = eMode;
    
    dev->devstate     = DEV_STATE_DISABLED;

    dev->nodehead     = NULL;

    dev->Reqreadyhead = NULL;
    dev->Reqreadytail = NULL;
    dev->Reqpendhead  = NULL;

    dev->Pollstate    = MBM_IDLE;
    
    dev->retry        = MBM_DEFAULT_RETRY_COUNT;
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

// must be delet all the request and node
void MbmRemove(uint8_t ucPort)
{
/*     MbmDev_t *srh = NULL;
     MbmDev_t *pre = NULL;
     
    if(mbm_dev_head == NULL)
        return;

    srh = mbm_dev_head;

    while(srh)
    {
        if(srh->port == ucPort)
            break;
        
        pre = srh;
        srh = srh->next;
    }

    if(srh){
        if(pre == NULL)
            mbm_dev_head = srh->next;
        else
            pre->next = srh->next;
        
        mb_free(srh);
    }*/
}

MbErrorCode_t MbmSetPara(MbmDev_t *dev, 
                                uint8_t retry,uint32_t replytimeout,
                                uint32_t delaypolltime, uint32_t broadcastturntime)
{
    if(dev == NULL)
        return MB_EINVAL;
    
    dev->retry = (retry > MBM_RETRY_COUNT_MAX) ? MBM_RETRY_COUNT_MAX : retry;
    
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
/* ����һ�������ڵ�ͼĴ����б� */
MbmNode_t *MbmNodeNew(uint8_t slaveaddr,
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
    MbReg_t *reg;
    MbmNode_t *node;

    /* check slave address valid */
    if(slaveaddr < MB_ADDRESS_MIN || slaveaddr > MB_ADDRESS_MAX)
        return NULL;
    
    node = (MbmNode_t *)mb_malloc(sizeof(MbmNode_t));
    if(node == NULL){
        return NULL;
    }
    lens = MbRegBufSizeCal(reg_holding_num,reg_input_num,reg_coils_num,reg_discrete_num);

    regbuf = (uint8_t *)mb_malloc(lens);
    if(regbuf == NULL){
        mb_free(node);
        return NULL;
    }
    reg = (MbReg_t *)&node->regs;
    
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

    return node;
}
void MbmNodeCallBackAssign(MbmNode_t *node, pfnReqResultCB cb, void *arg)
{
    if(node){
        node->cb = cb;
        node->arg = arg;
    }
}

/* �ͷŽڵ㣬�ͷ���MbmNodeNew�����Ľڵ��ڴ� */
void MbmNodeFree(MbmNode_t *node)
{
    if(node){
        if(node->regs.pReghold)
            mb_free(node->regs.pReghold);
        mb_free(node);
    }
}

/* ���ڵ���뵽�������ڵ���о�̬��̬���� */
MbErrorCode_t MbmAddNode(MbmDev_t *dev, MbmNode_t *node)
{
    MbmNode_t *srhnode;
    
    if(dev == NULL || node == NULL)
        return MB_EINVAL;
    
    /* check slave address valid */
    if(node->slaveaddr < MB_ADDRESS_MIN || node->slaveaddr > MB_ADDRESS_MAX)
        return MB_EILLNODEADDR;
    
    srhnode = MbmSearchNode(dev,node->slaveaddr);
    if(srhnode)
        return MB_ENODEEXIST;
    
    if(dev->nodehead == NULL){
        dev->nodehead = node;
    }
    else{
        node->next = dev->nodehead;
        dev->nodehead = node;
    }

    return MB_ENOERR;
}
/* ���ڵ������ɾ�� */
MbErrorCode_t MbmRemoveNode(MbmDev_t *dev, uint8_t slaveaddr)
{
    MbmNode_t *srchnode;
    MbmNode_t *prenode;
    
    if( dev == NULL )
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
    }
    
    return MB_ENOERR;
}

/* search node on the host list ? */
MbmNode_t *MbmSearchNode(MbmDev_t *dev, uint8_t slaveaddr)
{
    MbmNode_t *srh;

    if(dev == NULL)
        return NULL;

    srh = dev->nodehead;

    while(srh)
    {
        if(srh->slaveaddr == slaveaddr)
            break;

        srh = srh->next;
    }

    return srh;
}


MbErrorCode_t MbmvStart(MbmDev_t *dev)
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

MbErrorCode_t MbmStop(MbmDev_t *dev)
{
    if( dev->devstate == DEV_STATE_NOT_INITIALIZED )
        return MB_EILLSTATE;

    if( dev->devstate == DEV_STATE_ENABLED ){
        dev->pMbStopCur(dev);
        dev->devstate = DEV_STATE_DISABLED;
    }

    return MB_ENOERR;
}

MbErrorCode_t MbmClose(MbmDev_t *dev)
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

 MbReqResult_t MbmSend(MbmDev_t *dev, MbmReq_t *req)
{   
    uint16_t crc_lrc;
    
    if(dev->currentMode == MB_RTU){
#if MB_RTU_ENABLED > 0
        crc_lrc = MbCRC16(req->padu,req->adulength);
        req->padu[req->adulength++] = crc_lrc & 0xff;
        req->padu[req->adulength++] = (crc_lrc >> 8) & 0xff;
#endif
    }else if(dev->currentMode == MB_ASCII){
#if MB_ASCII_ENABLED > 0
        crc_lrc = MbLRC(req->padu,req->adulength);
        req->padu[req->adulength++] = crc_lrc & 0xff;
#endif
    }else{
        /* tcp no check sum */
    }
    
    if(!req->scanrate) // if zero add ready list immediately but only once
        __masterReqreadylist_addtail(dev,req);
    else
        __masterReqpendlist_add(dev,req);

    return MBR_ENOERR;
}


void MbmPoll(void)
{
    static uint32_t HistimerCounter = 0;
    MbmDev_t *curdev;    
    uint32_t elapsedMSec = 0;

    elapsedMSec = (uint32_t)(MbSys_now() - HistimerCounter);
    if(elapsedMSec)
        HistimerCounter = MbSys_now();

    curdev = mbm_dev_head;
     while(curdev){
        __MbmHandle(curdev,elapsedMSec);
        curdev = curdev->next;
    }
}

static MbErrorCode_t __MbmHandle(MbmDev_t *dev,uint32_t timediff)
{
    uint8_t *pRemainFrame; // remain fram
    uint8_t ucFunctionCode;
    uint16_t usLength;
    MbException_t eException;
    MbReqResult_t result;
    MbHeader_t header;
    MbmReq_t *req;

    pMbmParseRspHandler handle;

    /* Check if the protocol stack is ready. */
    if( dev->devstate != DEV_STATE_ENABLED ){
        return MB_EILLSTATE;
    }
      
    switch (dev->Pollstate){
    case MBM_IDLE:
        dev->Delaypolltimecnt = 0;
        dev->Pollstate = MBM_DELYPOLL;
        break;
    case MBM_XMIT: 
        req = __masterReqreadylist_peek(dev);
        if(req && (dev->pMbSendCur(dev,req->padu,req->adulength) == MB_ENOERR)){
            dev->Pollstate = MBM_XMITING;
        }
        else{ /* nothing want to send or send error, wait a moment to try*/
            dev->Pollstate = MBM_DELYPOLL;
        }
        break;

    case MBM_RSPEXCUTE:        
        req = __masterReqreadylist_peek(dev);
        if(req == NULL) { /* some err happen ,then no request in list*/
            dev->Pollstate = MBM_DELYPOLL;
            break;
        }
    
        /* parser a adu fram */
        result = dev->pMbReceivedCur(dev, &header, &ucFunctionCode, &pRemainFrame, &usLength);
        if(result == MBR_ENOERR){
            
            /* not for us ,continue to wait response */
            if((req->funcode != (ucFunctionCode & 0x7f)) || (req->slaveaddr != header.introute.slaveid)){
                dev->Pollstate = MBM_WAITRSP;
                break;
            }
            
            /* funcoe and slaveid same, this frame for us and then excute it*/
            if(ucFunctionCode & 0x80){ // �쳣��
                eException = (MbException_t)pRemainFrame[0]; //�쳣��
            }
            else{
                result = MBR_EINFUNCTION;
                handle = MbmSearchCB(ucFunctionCode);
                if(handle)
                    result = handle(&req->node->regs, req->regaddr, req->regcnt, pRemainFrame, usLength); 

                
            }
        }
        if(result != MBR_ENOERR){
            req->errcnt++;
        }
        
        if(req->node->cb)
            req->node->cb(result, eException, req); //ִ�лص�
        
        if(result == MBR_EINFUNCTION){ // �޴˹����룬// remove from ready list
            __masterReqreadylist_removehead(dev); // remove from ready list
            MbmReqBufDelete(req); // delete request
        }
        else {
            __masterReqreadylist_removehead(dev); // remove from ready list
            if((req->slaveaddr == MB_ADDRESS_BROADCAST) || (req->scanrate == 0))// only once
                MbmReqBufDelete(req); 
            else{
                __masterReqpendlist_add(dev, req);// move to pend list
            }                
        } 
        dev->Pollstate = MBM_DELYPOLL;   
        break;
    case MBM_RSPTIMEOUT:
        req = __masterReqreadylist_peek(dev);
        if(req == NULL) { /* some err happen ,then no request in list*/
            dev->Pollstate = MBM_DELYPOLL;
        }
        else{
            req->errcnt++;
            if(req->node->cb)
                req->node->cb(result, eException, req); //ִ�лص�
            __masterReqreadylist_removehead(dev);
            if((req->slaveaddr == MB_ADDRESS_BROADCAST) || (req->scanrate == 0))// only once
                MbmReqBufDelete(req);
            else{// move to pend list
                __masterReqpendlist_add(dev, req);
            }
            dev->Pollstate = MBM_XMIT;
        }
        break;
    case MBM_BROADCASTTURN:
    case MBM_DELYPOLL:
    case MBM_XMITING:
        break;
    case MBM_WAITRSP:
        req = __masterReqreadylist_peek(dev);
        if(req == NULL) { /* some err happen ,then no request in list*/
            dev->Pollstate = MBM_DELYPOLL;
        }
        else if(req->slaveaddr == MB_ADDRESS_BROADCAST){           
            __masterReqreadylist_removehead(dev);
            MbmReqBufDelete(req);
            dev->Pollstate = MBM_BROADCASTTURN;
        }
        else{
            /* keep wait for responese */
        }
        break;        
    default:
        dev->Pollstate = MBM_IDLE;
    }
    
    if(timediff){
        switch (dev->Pollstate){
        case MBM_BROADCASTTURN:
            /* �㲥ת���ӳ�ʱ�� ,�����㲥����ڵ㴦���ʱ��*/
            dev->Broadcastturntimecnt += timediff;
            if(dev->Broadcastturntimecnt >= dev->Broadcastturntime){
                dev->Broadcastturntimecnt = 0;                
                dev->Pollstate = MBM_XMIT;
            }
            break;
        case MBM_DELYPOLL:
            /* ��������֮����ӳ�ʱ�� */
            dev->Delaypolltimecnt += timediff;
            if(dev->Delaypolltimecnt >= dev->Delaypolltime){
                dev->Delaypolltimecnt = 0;                
                dev->Pollstate = MBM_XMIT;
            }
            break;
        case MBM_WAITRSP:
            dev->Replytimeoutcnt += timediff;
            if(dev->Replytimeoutcnt >= dev->Replytimeout){
                dev->Replytimeoutcnt = 0;
                dev->Pollstate = MBM_RSPTIMEOUT;
            }
            break;
        default:
            break;
        }

        __masterReqpendlistScan(dev,timediff);// scan pend list 
    }
    
    return MB_ENOERR;
}

/*## �����������б�β������һ������ ���Ǹ�fifo�Ķ���*/
static MbErrorCode_t __masterReqreadylist_addtail(MbmDev_t *dev, MbmReq_t *req)
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
static MbmReq_t *__masterReqreadylist_peek(MbmDev_t *dev)
{
    return dev->Reqreadyhead;
}

static void __masterReqreadylist_removehead(MbmDev_t *dev)
{
    if(dev == NULL || dev->Reqreadyhead == NULL) /* nothing to remove */ 
        return;
    
    dev->Reqreadyhead = dev->Reqreadyhead->next;
    if(dev->Reqreadyhead == NULL) /* it is reach tail and noting in the list */
        dev->Reqreadytail = NULL;
}


/*## �����������б�����һ������ */
static MbErrorCode_t __masterReqpendlist_add(MbmDev_t *dev, MbmReq_t *req)
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
static void __masterReqpendlistScan(MbmDev_t *dev, uint32_t diff)
{
    MbmReq_t *prevReq;
    MbmReq_t *srchReq;
    MbmReq_t *tempReq;
    
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
static MbErrorCode_t __masterdev_add(MbmDev_t *dev)
{
    
    if(__masterdev_search(dev->port))
        return MB_EDEVEXIST;

    if(mbm_dev_head == NULL){
        dev->next = NULL;
        mbm_dev_head = dev;
    }
    else{
        dev->next = mbm_dev_head;
        mbm_dev_head = dev;
    }
    
    return MB_ENOERR;
}

/*ok 
 * RTU �� ASCII ��Ӳ������Ψһ�ģ������ظ�
 * TCP service �˿�Ҳ��Ψһ��
 */
static MbmDev_t *__masterdev_search(uint8_t port)
{
    MbmDev_t *srh = NULL;
    
    if(mbm_dev_head == NULL)
        return NULL;

    srh = mbm_dev_head;

    while(srh)
    {
        if(srh->port == port)
            return srh;

        srh = srh->next;
    }

    return NULL;
}


#endif

