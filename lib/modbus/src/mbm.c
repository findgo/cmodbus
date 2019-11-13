#include "mbconfig.h"

#if (MB_RTU_ENABLED > 0 || MB_ASCII_ENABLED > 0 || MB_TCP_ENABLED > 0) && MB_MASTER_ENABLED > 0

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
#include "mem.h"
#include "mbmbuf.h"
#include "mbfunc.h"
#include "mb.h"
#include "port.h"

static MbErrorCode_t __MbmHandle(MbmDev_t *dev, uint32_t timediff);

static void __MbmReqPendlistScan(MbmDev_t *dev, uint32_t diff);

static MbmDev_t *__MbmdevSearch(uint8_t port);

static MsgQ_t mbm_dev_head = NULL;

//static msgboxstatic_t msgboxHandlebuf = MSGBOX_STATIC_INIT(MSGBOX_UNLIMITED_CAP);

MbmHandle_t MbmNew(MbMode_t eMode, uint8_t ucPort, uint32_t ulBaudRate, MbParity_t eParity) {
    MbErrorCode_t eStatus = MB_ENOERR;
    MbmDev_t *dev;

    if ((dev = (MbmDev_t *) MsgAlloc(sizeof(MbmDev_t))) == NULL)
        return NULL;

    memset(dev, 0, sizeof(MbmDev_t));

    switch (eMode) {
#if MB_RTU_ENABLED > 0
        case MB_RTU:
            dev->pStartCur = MbmRTUStart;
            dev->pStopCur = MbmRTUStop;
            dev->pCloseCur = MbmRTUClose;
            dev->pSendCur = MbmRTUSend;
            dev->pMbReceivedCur = MbmRTUReceive;

            eStatus = MbmRTUInit(dev, ucPort, ulBaudRate, eParity);
            break;
#endif

#if MB_ASCII_ENABLED > 0
        case MB_ASCII:
            dev->pStartCur = MbmASCIIStart;
            dev->pStopCur = MbmASCIIStop;
            dev->pCloseCur = MbmASCIIClose;
            dev->pSendCur = MbmASCIISend;
            dev->pMbReceivedCur = MbmASCIIReceive;

            eStatus = MbmASCIIInit(dev, ucPort, ulBaudRate, eParity);
            break;
#endif

        default:
            eStatus = MB_EINVAL;
            break;

    }

    if (eStatus != MB_ENOERR) {
        MsgDealloc(dev);
        return NULL;
    }

    // init parameter
    dev->port = ucPort;
    dev->mode = eMode;
    dev->T50PerCharater = (ulBaudRate > 19200) ? 10 : (220000 / ulBaudRate);

    dev->state = DEV_STATE_DISABLED;

    dev->nodehead = NULL;

    dev->Reqreadyhead = NULL;
    dev->Reqpendinghead = NULL;

    dev->Pollstate = MBM_IDLE;

    dev->retry = MBM_DEFAULT_RETRY_COUNT;
    dev->retrycnt = 0;

    dev->XmitingTime = 0;
    dev->Replytimeout = MBM_DEFAULT_REPLYTIMEOUT;
    dev->Replytimeoutcnt = 0;
    dev->Delaypolltime = MBM_DEFAULT_DELAYPOLLTIME;
    dev->Delaypolltimecnt = 0;
    dev->Broadcastturntime = MBM_DEFAULT_BROADTURNTIME;
    dev->Broadcastturntimecnt = 0;

    // check the device on the list ?
    if (__MbmdevSearch(dev->port)) {
        MsgDealloc(dev);

        return NULL;
    }
    MsgQPutFront(&mbm_dev_head, dev);
    return (MbmHandle_t) dev;
}

// must be delete all the request and node
//不实现
void MbmFree(uint8_t ucPort) {
/*
    MbmDev_t *srh = NULL;
    MbmDev_t *pre = NULL;

    msgQ_for_each_msg(&mbm_dev_head, srh){
        if(srh->port == ucPort)
            break;

        pre = srh; // save previous msg      
    }
    
    if(srh){
        MsgQExtract(&mbm_dev_head, srh, pre);
        MsgDealloc(srh);
    }
    */
}

MbErrorCode_t MbmSetPara(MbmHandle_t dev, uint8_t retry, uint32_t replytimeout,
                         uint32_t delaypolltime, uint32_t broadcastturntime) {
    MbmDev_t *pdev = (MbmDev_t *) dev;

    if (pdev == NULL)
        return MB_EINVAL;

    pdev->retry = (retry > MBM_RETRY_COUNT_MAX) ? MBM_RETRY_COUNT_MAX : retry;

    if (replytimeout < MBM_REPLYTIMEOUT_MIN)
        pdev->Replytimeout = MBM_REPLYTIMEOUT_MIN;
    else if (replytimeout > MBM_REPLYTIMEOUT_MAX)
        pdev->Replytimeout = MBM_REPLYTIMEOUT_MAX;
    else
        pdev->Replytimeout = replytimeout;

    if (delaypolltime < MBM_DELAYPOLLTIME_MIN)
        pdev->Delaypolltime = MBM_DELAYPOLLTIME_MIN;
    else if (delaypolltime > MBM_DELAYPOLLTIME_MAX)
        pdev->Delaypolltime = MBM_DELAYPOLLTIME_MAX;
    else
        pdev->Delaypolltime = delaypolltime;

    if (broadcastturntime < MBM_DELAYPOLLTIME_MIN)
        pdev->Broadcastturntime = MBM_BROADTURNTIME_MIN;
    else if (broadcastturntime > MBM_BROADTURNTIME_MAX)
        pdev->Broadcastturntime = MBM_BROADTURNTIME_MAX;
    else
        pdev->Broadcastturntime = broadcastturntime;

    return MB_ENOERR;
}

/* 创建一个独立节点和寄存器列表 */
MbmNode_t *MbmNodeNew(uint8_t slaveID,
                      uint16_t holdingAddrStart, uint16_t holdingNum,
                      uint16_t inputAddrStart, uint16_t inputNum,
                      uint16_t coilsAddrStart, uint16_t coilsNum,
                      uint16_t discreteAddrStart, uint16_t discreteNum) {
    uint32_t lens;
    uint8_t *regbuf;
    MbReg_t *reg;
    MbmNode_t *node;

    /* check slave address valid */
    if (slaveID < MB_ADDRESS_MIN || slaveID > MB_ADDRESS_MAX)
        return NULL;

    if ((node = (MbmNode_t *) MsgAlloc(sizeof(MbmNode_t))) == NULL) {
        return NULL;
    }
    memset(node, 0, sizeof(MbmNode_t));

    lens = MbRegBufSizeCal(holdingNum, inputNum, coilsNum, discreteNum);
    regbuf = (uint8_t *) KMalloc(lens);
    if (regbuf == NULL) {
        MsgDealloc(node);
        return NULL;
    }
    memset(regbuf, 0, lens);

    reg = (MbReg_t *) &node->pRegs;

    reg->holdingAddrStart = holdingAddrStart;
    reg->holdingNum = holdingNum;

    reg->inputAddrStart = inputAddrStart;
    reg->inputNum = inputNum;

    reg->coilsAddrStart = coilsAddrStart;
    reg->coilsNum = coilsNum;

    reg->discreteAddrStart = discreteAddrStart;
    reg->discreteNum = discreteNum;

    reg->pHolding = (uint16_t *) &regbuf[0];
    lens = holdingNum * sizeof(uint16_t);

    reg->pInput = (uint16_t *) &regbuf[lens];
    lens += inputNum * sizeof(uint16_t);

    reg->pCoil = (uint8_t *) &regbuf[lens];
    lens += (coilsNum >> 3) + (((coilsNum & 0x07) > 0) ? 1 : 0);

    reg->pDiscrete = (uint8_t *) &regbuf[lens];

    node->slaveID = slaveID;

    return node;
}

// 给节点安排一个回调函数
void MbmNodeCallBackAssign(MbmNode_t *node, pfnReqResultCB cb, void *arg) {
    if (node) {
        node->cb = cb;
        node->arg = arg;
    }
}

/* 释放节点，释放由MbmNodeNew创建的节点内存 */
void MbmNodeFree(MbmNode_t *node) {
    if (node) {
        if (node->pRegs.pHolding)
            KFree(node->pRegs.pHolding);
        MsgFree(node);
    }
}

/* 将节点加入到主机，由MbmNodeNew创建的节点 */
MbErrorCode_t MbmAddNode(MbmHandle_t dev, MbmNode_t *node) {
    MbmNode_t *srhnode;

    if (dev == NULL || node == NULL)
        return MB_EINVAL;

    /* check slave address valid */
    if (node->slaveID<MB_ADDRESS_MIN || node->slaveID>MB_ADDRESS_MAX)
        return MB_EILLNODEADDR;

    // check node on the list?
    srhnode = MbmSearchNode(dev, node->slaveID);
    if (srhnode)
        return MB_ENODEEXIST;

    MsgQPutFront(&(((MbmDev_t *) dev)->nodehead), node);

    return MB_ENOERR;
}

/* 将节点从主机删除 */
MbErrorCode_t MbmRemoveNode(MbmHandle_t dev, uint8_t slaveID) {
    MbmNode_t *srchnode;
    MbmNode_t *prenode = NULL;

    if (dev == NULL)
        return MB_EINVAL;

    MsgQ_for_each_msg(&(((MbmDev_t *) dev)->nodehead), srchnode) {
        if (srchnode->slaveID == slaveID) // find it
            break;

        prenode = srchnode;
    }

    if (srchnode) {
        //first remove from node list
        MsgQExtract(&(((MbmDev_t *) dev)->nodehead), srchnode, prenode);
    }
    // init
    srchnode->slaveID = 0;

    return MB_ENOERR;
}

/* search node on the host list ? */
MbmNode_t *MbmSearchNode(MbmHandle_t dev, uint8_t slaveID) {
    MbmNode_t *srh;

    if (dev == NULL)
        return NULL;

    MsgQ_for_each_msg(&(((MbmDev_t *) dev)->nodehead), srh) {
        if (srh->slaveID == slaveID)
            break;
    }

    return srh;
}


MbErrorCode_t MbmStart(MbmHandle_t dev) {
    MbmDev_t *pdev = (MbmDev_t *) dev;

    if (pdev->state == DEV_STATE_NOT_INITIALIZED)
        return MB_EILLSTATE;

    if (pdev->state == DEV_STATE_DISABLED) {
        /* Activate the protocol stack. */
        pdev->pStartCur(dev);
        pdev->state = DEV_STATE_ENABLED;
    }

    return MB_ENOERR;
}

MbErrorCode_t MbmStop(MbmHandle_t dev) {
    MbmDev_t *pdev = (MbmDev_t *) dev;

    if (pdev->state == DEV_STATE_NOT_INITIALIZED)
        return MB_EILLSTATE;

    if (pdev->state == DEV_STATE_ENABLED) {
        pdev->pStopCur(dev);
        pdev->state = DEV_STATE_DISABLED;
    }

    return MB_ENOERR;
}

MbErrorCode_t MbmClose(MbmHandle_t dev) {
    MbmDev_t *pdev = (MbmDev_t *) dev;

    // must be stop first then it can close
    if (pdev->state == DEV_STATE_DISABLED) {
        if (pdev->pCloseCur != NULL) {
            pdev->pCloseCur(dev);
        }

        return MB_ENOERR;
    }

    return MB_EILLSTATE;
}

MbReqResult_t MbmSend(MbmHandle_t dev, MbmReq_t *req) {
    uint16_t crc_lrc;
    MbmDev_t *pdev = (MbmDev_t *) dev;

    if (pdev->mode == MB_RTU) {
#if MB_RTU_ENABLED > 0
        crc_lrc = MbCRC16(req->adu, req->adulength);
        req->adu[req->adulength++] = crc_lrc & 0xff;
        req->adu[req->adulength++] = (crc_lrc >> 8) & 0xff;
#endif
    } else if (pdev->mode == MB_ASCII) {
#if MB_ASCII_ENABLED > 0
        crc_lrc = MbLRC(req->adu,req->adulength);
        req->adu[req->adulength++] = crc_lrc & 0xff;
#endif
    } else {
        /* tcp no check sum */
    }

    if (req->scanrate) {
        MsgQPut(&(pdev->Reqpendinghead), req);
    } else {
        // if zero add ready list immediately but only once
        MsgQPut(&(pdev->Reqreadyhead), req);
    }

    return MBR_ENOERR;
}

// TODO:  retry cnt 
void MbmPoll(void) {
    static uint32_t HistimerCounter = 0;
    MbmDev_t *curdev;
    uint32_t diff = 0;
    uint32_t tms;

    tms = MbSys_now();

    if (tms != HistimerCounter) {
        diff = (uint32_t) ((MbSys_now() - HistimerCounter) & 0xffffffffu);
        HistimerCounter = tms;
    }

    MsgQ_for_each_msg(&mbm_dev_head, curdev) {
        __MbmHandle(curdev, diff);
    }
}

static MbErrorCode_t __MbmHandle(MbmDev_t *dev, uint32_t timediff) {
    uint8_t *pRemainFrame; // remain fram
    uint8_t functionCode;
    uint16_t usLength;
    MbException_t exception;
    MbReqResult_t result;
    MbHeader_t header;
    MbmReq_t *req;

    pMbmParseRspHandler handle;

    /* Check if the protocol stack is ready. */
    if (dev->state != DEV_STATE_ENABLED) {
        return MB_EILLSTATE;
    }

    switch (dev->Pollstate) {
        case MBM_IDLE:
            dev->Delaypolltimecnt = 0;
            dev->Pollstate = MBM_DELYPOLL;
            break;
        case MBM_XMIT:
            req = MsgQPeek(&(dev->Reqreadyhead)); // peek ready list ,any request on the list?
            if (req && (dev->pSendCur(dev, req->adu, req->adulength) == MB_ENOERR)) {
                if (dev->mode == MB_RTU) {
                    dev->XmitingTime = dev->T50PerCharater * 50 * req->adulength / 1000 + 1;
                } else {
                    dev->XmitingTime = dev->T50PerCharater * 50 * (req->adulength + 1) * 2 / 1000;
                }
                dev->Replytimeoutcnt = 0;
                dev->Pollstate = MBM_WAITRSP;
            } else { /* nothing want to send or send error, wait a moment to try*/
                dev->Pollstate = MBM_DELYPOLL;
            }
            break;

        case MBM_RSPEXCUTE:  // response excute
            req = MsgQPop(&(dev->Reqreadyhead));// pop from ready list
            if (req == NULL) { /* some err happen ,then no request in list*/
                dev->Pollstate = MBM_DELYPOLL;
                break;
            }

            /* parser a adu fram */
            result = dev->pMbReceivedCur(dev, &header, &functionCode, &pRemainFrame, &usLength);
            if (result == MBR_ENOERR) {
                /* not for us ,continue to wait response */
                if ((req->funcode != (functionCode & 0x7f)) || (req->slaveID != header.introute.slaveID)) {
                    dev->Pollstate = MBM_WAITRSP;
                    break;
                }

                /* funcode and slaveID same, this frame for us and then excute it*/
                if (functionCode & 0x80) { // 异常
                    result = MBR_ERSPEXCEPTOIN;
                    exception = (MbException_t) pRemainFrame[0]; //异常码
                } else {
                    result = MBR_EINFUNCTION;
                    handle = MbmFuncHandleSearch(functionCode);
                    if (handle)
                        result = handle(&req->node->pRegs, req->regaddr, req->regcnt, pRemainFrame, usLength);
                }
            }

            // response exception is not a error
            if (result != MBR_ENOERR && result != MBR_ERSPEXCEPTOIN) {
                req->errcnt++;
            }

            if (req->node->cb)
                req->node->cb(result, exception, req); //执行回调

            if (result == MBR_EINFUNCTION) { // 无此功能码
                MbmReqMsgDelete(req); // delete request
            } else {
                if ((req->slaveID == MB_ADDRESS_BROADCAST) || (req->scanrate == 0)) {// only once
                    MbmReqMsgDelete(req);
                } else { // move to pend list
                    MsgQPut(&(dev->Reqpendinghead), req);
                }
            }
            dev->Pollstate = MBM_DELYPOLL;
            break;
        case MBM_RSPTIMEOUT:
            req = MsgQPop(&(dev->Reqreadyhead));
            if (req == NULL) { /* some err happen ,then no request in list*/
                dev->Pollstate = MBM_DELYPOLL;
            } else {
                result = MBR_ETIMEOUT;
                req->errcnt++;
                if (req->node->cb)
                    req->node->cb(result, MB_EX_NONE, req); //执行回调

                if ((req->slaveID == MB_ADDRESS_BROADCAST) || (req->scanrate == 0))// only once
                    MbmReqMsgDelete(req);
                else {// move to pend list
                    MsgQPut(&(dev->Reqpendinghead), req);
                }
                dev->Pollstate = MBM_XMIT;
            }
            break;
        case MBM_BROADCASTTURN:
        case MBM_DELYPOLL:
            break;
        case MBM_WAITRSP:  // send ok ? wait for server response
            req = MsgQPeek(&(dev->Reqreadyhead));
            if (req == NULL) { /* some err happen ,then no request in list*/
                dev->Pollstate = MBM_DELYPOLL;
            } else if (req->slaveID == MB_ADDRESS_BROADCAST) { // broadcast ,remove from pend list soon
                MsgQPop(&(dev->Reqreadyhead));
                MbmReqMsgDelete(req);
                dev->Pollstate = MBM_BROADCASTTURN;
            } else {
                /* keep wait for responese */
            }
            break;
        default:
            dev->Pollstate = MBM_IDLE;
    }

    if (timediff) {

        switch (dev->Pollstate) {
            case MBM_BROADCASTTURN: /* 广播转换延迟时间 ,发出广播后给节点处理的时间*/
                dev->Broadcastturntimecnt += timediff;
                if (dev->Broadcastturntimecnt >= dev->Broadcastturntime) {
                    dev->Broadcastturntimecnt = 0;
                    dev->Pollstate = MBM_XMIT;
                }
                break;

            case MBM_DELYPOLL: /* 两个请求之间的延迟时间, 请求失败或重试时的延迟*/
                dev->Delaypolltimecnt += timediff;
                if (dev->Delaypolltimecnt >= dev->Delaypolltime) {
                    dev->Delaypolltimecnt = 0;
                    dev->Pollstate = MBM_XMIT;
                }
                break;

            case MBM_WAITRSP: /* 等待应答超时时间 */
                dev->Replytimeoutcnt += timediff;
                if (dev->Replytimeoutcnt >= (dev->Replytimeout + dev->XmitingTime)) {
                    dev->Replytimeoutcnt = 0;
                    dev->Pollstate = MBM_RSPTIMEOUT;
                }
                break;

            default:
                break;
        }

        __MbmReqPendlistScan(dev, timediff);// scan pend list 
    }

    return MB_ENOERR;
}


/*扫描挂起列表         发现有准备好的请求 移入就绪列表      */
static void __MbmReqPendlistScan(MbmDev_t *dev, uint32_t diff) {
    MbmReq_t *prevReq = NULL;
    MbmReq_t *srchReq;

    MsgQ_for_each_msg(&(dev->Reqpendinghead), srchReq) {
        srchReq->scancnt += diff;
        if (srchReq->scancnt >= srchReq->scanrate) {// scan timeout move to ready list

            srchReq->scancnt = 0; // clear scan count
            //first remove from pend list
            MsgQExtract(&(dev->Reqpendinghead), srchReq, prevReq);
            // then add to read list tail
            MsgQPut(&(dev->Reqreadyhead), srchReq);
        } else {
            prevReq = srchReq;
        }
    }
}

/*ok */
static MbmDev_t *__MbmdevSearch(uint8_t port) {
    MbmDev_t *srh = NULL;

    MsgQ_for_each_msg(&mbm_dev_head, srh) {
        if (srh->port == port)
            break;
    }

    return srh;
}


#endif

