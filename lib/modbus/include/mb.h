#ifndef __MB_H_
#define __MB_H_

#include "mbconfig.h"
#include "mbproto.h"
#include "msglink.h"

// request fail retry 
#define MBM_DEFAULT_RETRY_COUNT        0        /* default request failed to retry count */
#define MBM_RETRY_COUNT_MAX            3        /* every request retry count */

/* for master defined (units of millisecond)*/
#define MBM_DEFAULT_REPLYTIMEOUT    1000        /* response timeout */
#define MBM_REPLYTIMEOUT_MIN        50          /* response timeout min*/
#define MBM_REPLYTIMEOUT_MAX        60000       /* response timeout max*/

#define MBM_DEFAULT_DELAYPOLLTIME   10          /* delay time between polls */
#define MBM_DELAYPOLLTIME_MIN       1           /* delay time between polls */
#define MBM_DELAYPOLLTIME_MAX       1000        /* delay time between polls */

#define MBM_DEFAULT_BROADTURNTIME   100         /* after broadcast turn round time */
#define MBM_BROADTURNTIME_MIN       50          /* after broadcast turn round time */
#define MBM_BROADTURNTIME_MAX       200         /* after broadcast turn round time */

#define MBM_DEFAULT_SCANRATE        1000        /* every request scan rate */
#define MBM_SCANRATE_MAX            60000       /* every request scan rate */

// port defined
enum {
    MBCOM0 = 0,
    MBCOM1,
    MBCOM2,
    MBCOM3,
    MBCOM4,
    MBCOM5,
    MBCOM6,
    MBCOM7,
    MBCOM8,
};

/*\brief Parity used for characters in serial mode.*/
typedef enum {
    MB_PAR_NONE,                /*!< No parity. */
    MB_PAR_ODD,                 /*!< Odd parity. */
    MB_PAR_EVEN                 /*!< Even parity. */
} MbParity_t;

/*vbrief Modbus serial transmission modes .*/
typedef enum {
    MB_RTU,     /*!< RTU transmission mode. */
    MB_ASCII,   /*!< ASCII transmission mode. */
    MB_TCP      /*!< TCP mode. */
} MbMode_t;

// device state
typedef enum {
    DEV_STATE_NOT_INITIALIZED = 0,
    DEV_STATE_DISABLED,
    DEV_STATE_ENABLED
} MbDevState_t;
/* \brief Errorcodes used by all function in the protocol stack.*/
typedef enum {
    MB_ENOERR,                  /*!< no error. */
    MB_ENOREG,                  /*!< illegal register address. */
    MB_EINVAL,                  /*!< illegal argument. */
    MB_EPORTERR,                /*!< porting layer error. */
    MB_ENORES,                  /*!< insufficient resources. */
    MB_EIO,                     /*!< I/O error. */
    MB_EILLSTATE,               /*!< protocol stack in illegal state. */
    MB_ETIMEDOUT,               /*!< timeout error occurred. */
    MB_EDEVEXIST,               // device exist
    MB_EILLNODEADDR,            // invalid node address
    MB_ENODEEXIST,             /*!< node exist */
} MbErrCode_t;

// 定义寄存器属性  
typedef struct {
    uint16_t holdingAddrStart;
    uint16_t inputAddrStart;
    uint16_t coilsAddrStart;
    uint16_t discreteAddrStart;
    uint16_t holdingNum;
    uint16_t inputNum;
    uint16_t coilsNum;
    uint16_t discreteNum;
    uint16_t *pHolding;
    uint16_t *pInput;
    uint8_t *pCoil;
    uint8_t *pDiscrete;
} MbReg_t;

//定义modbus头部 
typedef struct {
    uint16_t tid;       // TCP : Transaction Identifier
    uint16_t pid;       // TCP : Protocol Identifier default : 0
    uint16_t length;    // TCP : Number of bytes UID+ PDU length
    union {
        uint8_t uid;    // TCP: same as slave ID
        uint8_t slaveID; // slave ID
    } inRoute;
} MbHeader_t;

// 主机设备句柄
typedef void *MbmHandle_t;

typedef void (*pActionHandle)(MbmHandle_t dev);

/************************************** define for master ******************************************/
// 定义请求错误枚举
typedef enum {
    MBR_ENOERR,           /* no error */
    MBR_EINNODEADDR,      /* illegal slave address*/
    MBR_ENODENOSETUP,     /* node not yet established */
    MBR_ENOREG,           /*!< illegal register address. */
    MBR_EINVAL,           /*!< illegal argument. */
    MBR_ENOMEM,           /* Out of memory */
    MBR_BUSY,             /* IO busy */
    MBR_EINFUNCTION,      /* no valid function */
    MBR_ETIMEOUT,         /* response timeout */
    MBR_MISSBYTE,         /* response receive miss byte */
    MBR_ECHECK,           /* response receive crc/lrc error */
    MBR_EREGDIFF,         /* response register address different from request */
    MBR_ERSPEXCEPTOIN,    /* response exception */
} MbReqResult_t;
// 定义轮询状态机
typedef enum {
    MBM_IDLE,      // 空闲
    MBM_DELYPOLL,  // 轮询延时
    MBM_BROADCASTTURN, // 广播
    MBM_XMIT,           // 发送
    MBM_WAITRSP,        // 等待应答
    MBM_RSPEXCUTE,      // 应答帧处理
    MBM_RSPTIMEOUT     // 应答超时
} MbmPollState_t;

// 应答寄存器解析
typedef MbReqResult_t (*pMbmParseRspHandler)(MbReg_t *regs,
                                             uint16_t ReqRegAddr, uint16_t ReqRegcnt,
                                             uint8_t *premain, uint16_t remainLength);

typedef MbReqResult_t (*pActionMasterReceive)(MbmHandle_t dev, MbHeader_t *phead, uint8_t *pfunCode, uint8_t **premain,
                                              uint16_t *premainLength);

typedef MbReqResult_t (*pActionMasterSend)(MbmHandle_t dev, const uint8_t *pAdu, uint16_t usAduLength);

//result may only error MBR_ENOERR,MBR_MISSBYTE, MBR_ECHECK, MBR_EINFUNCTION,MBR_EINVAL,MBR_ETIMEOUT,MBR_ERSPEXCEPTOIN
// eException : when result is MBR_ERSPEXCEPTOIN use is
// req :　mark request
typedef void (*pfnReqResultCB)(MbReqResult_t result, MbException_t eException, void *req);

// 节点描述
typedef struct {
    uint8_t slaveaddr;      // slave address 
    uint8_t reserved0;      // reserved
    uint16_t reserved1;     // reserved
    MbReg_t regs;           // The register table
    pfnReqResultCB cb;      // call back function
    void *arg;              // arg for callback function
} MbmNode_t;
// 请求描述
typedef struct {
    MbmNode_t *node;        /* mark the node */
    uint32_t errcnt;        /* request error count*/
    uint8_t slaveaddr;      /* mark slave address */
    uint8_t funcode;        /* mark function code */
    uint16_t regaddr;       /* mark reg address for rsp used */
    uint16_t regcnt;        /* mark reg count for rsp used */
    uint16_t scancnt;       /* scan time cnt */
    uint16_t scanrate;      /* scan rate  if 0 : once,  other request on scan rate */
    uint8_t adulength;     /* mark adu length*/
    uint8_t adu[];          /* mark adu for repeat send */
} MbmReq_t;
// 主机设备描述
typedef struct {
    uint8_t port;
    MbDevState_t devstate;      // device current state
    MbMode_t mode;       // work mode as RTU ASCII TCP 
    uint8_t Pollstate;          // poll state

    uint32_t T50PerCharater;  //发送一个字符所需时间 timevalueT50us = Ticks_per_1s / ( Baudrate / 11 ) = 220000 / Baudrate

    MsgQ_t nodehead;           /* slave node list on this host */

    MsgQ_t Reqreadyhead;        /* 就绪表 request ready list head*/
    MsgQ_t Reqpendinghead;      /* 挂起表 request suspend list */

    uint8_t retry;              // retry MAX count
    uint8_t retrycnt;           // retry count

    uint16_t XmitingTime;

    uint16_t Replytimeout;              /* response timeout */
    uint16_t Replytimeoutcnt;           /* response timeout count*/
    uint16_t Delaypolltime;             /* delay time between polls */
    uint16_t Delaypolltimecnt;          /* delay time between polls count*/
    uint16_t Broadcastturntime;         /* after broadcast turn round time */
    uint16_t Broadcastturntimecnt;      /* after broadcast turn round time count*/

    pActionHandle pMbStartCur;
    pActionHandle pMbStopCur;
    pActionHandle pMbCloseCur;
    pActionMasterSend pMbSendCur;
    pActionMasterReceive pMbReceivedCur;

    /* low layer use */
    volatile uint8_t AsciiBytePos; // only for ascii
    volatile uint8_t sendRcvState;
    volatile uint16_t sendAduBufCount;
    volatile uint16_t sendAduBufPos;
    volatile uint16_t rcvAduBufPos;
    volatile uint8_t AduBuf[MB_ADU_SIZE_MAX];
} MbmDev_t;

#define MbmSetPollMode(pDev, state)     do {pDev->Pollstate = state;}while(0)

MbReqResult_t MbmSend(MbmHandle_t dev, MbmReq_t *req);

/************************************** define for slave ******************************************/
// 定义adu帧 解析
typedef struct {
    MbHeader_t hdr;
    uint8_t functionCode;       // pdu function
    uint16_t pduFrameLength;    // pdu frame length
    uint8_t *pPduFrame;         // pdu frame
} MbsAduFrame_t;

//从机设备句柄
typedef void *MbsHandle_t;

typedef MbException_t (*pMbsFunctionHandler)(MbReg_t *pRegs, uint8_t *pPdu, uint16_t *pLength);

typedef MbErrCode_t (*pActionSlaveReceiveParse)(MbsHandle_t dev, MbsAduFrame_t *pAduFramePkt);

typedef MbErrCode_t (*pActionSlaveSend)(MbsHandle_t dev, uint8_t slaveID, const uint8_t *pPdu, uint16_t length);

// 从机设备描述
typedef struct {
    uint16_t port;
    uint8_t inuse;
    uint8_t reserved0;

    uint8_t slaveID;
    MbMode_t mode;           // work mode as RTU ASCII TCP?
    MbDevState_t state;   // device state
    uint8_t eventInFlag;    // for event?

    MbReg_t regs;            // register list

    pActionHandle pStartCur;
    pActionHandle pStopCur;
    pActionHandle pCloseCur;
    pActionSlaveReceiveParse pReceiveParseCur;
    pActionSlaveSend pSendCur;

    /* low layer use */
    volatile uint8_t AsciiBytePos; // only for ascii
    volatile uint8_t sendRcvState;
    volatile uint16_t sendAduBuffCount;
    volatile uint16_t sendAduBuffPos;
    volatile uint16_t rcvAduBuffPos;
    volatile uint8_t AduBuf[MB_ADU_SIZE_MAX];
} MbsDev_t;

// 发送个信号
#define MbsSemGive(pDev) do { pDev->eventInFlag = true;}while(0)

#endif
