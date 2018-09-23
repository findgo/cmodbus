#ifndef __MB_H_
#define __MB_H_

#include "mbconfig.h"
#include "mbproto.h"

#define MBM_DEFAULT_RETRY_COUNT        0          /* default request failed to retry count */
#define MBM_RETRY_COUNT_MAX            6
/* for master defined (units of millisecond)*/
#define MBM_DEFAULT_REPLYTIMEOUT    1000     /* response timeout */
#define MBM_DEFAULT_DELAYPOLLTIME   10         /* delay time between polls */
#define MBM_DEFAULT_BROADTURNTIME   100        /* after broadcast turn round time */
#define MBM_DEFAULT_SCANRATE        1000        /* every request scan rate */

#define MBM_REPLYTIMEOUT_MIN    50         /* response timeout min*/
#define MBM_REPLYTIMEOUT_MAX    60000      /* response timeout max*/
#define MBM_DELAYPOLLTIME_MIN   1          /* delay time between polls */
#define MBM_DELAYPOLLTIME_MAX   1000       /* delay time between polls */
#define MBM_BROADTURNTIME_MIN   50         /* after broadcast turn round time */
#define MBM_BROADTURNTIME_MAX   200        /* after broadcast turn round time */
#define MBM_SCANRATE_MAX        60000      /* every request scan rate */

// port defined
enum 
{
    MBCOM0 = 0,
    MBCOM1,
    MBCOM2,
    MBCOM3,
    MBCOM4,
    MBCOM5,
    MBCOM6,
    MBCOM7,
    MBCOM8,
    MBCOM9,
    MBCOM10,
    MBCOM11,
    MBCOM12,
    MBCOM13,
    MBCOM14,
    MBCOM15
};

/*! \ingroup modbus
 * \brief Parity used for characters in serial mode.
 */
typedef enum
{
    MB_PAR_NONE,                /*!< No parity. */
    MB_PAR_ODD,                 /*!< Odd parity. */
    MB_PAR_EVEN                 /*!< Even parity. */
}MbParity_t;
/*! \ingroup modbus
 * \brief Modbus serial transmission modes (RTU/ASCII).
 */
typedef enum
{
    MB_RTU,     /*!< RTU transmission mode. */
    MB_ASCII,   /*!< ASCII transmission mode. */
    MB_TCP      /*!< TCP mode. */
} MbMode_t;
// 
typedef enum
{
    DEV_STATE_NOT_INITIALIZED = 0,
    DEV_STATE_DISABLED,
    DEV_STATE_ENABLED
}MbDevState_t;
/*! \ingroup modbus
 * \brief Errorcodes used by all function in the protocol stack.
 */
typedef enum
{
    MB_ENOERR,                  /*!< no error. */
    MB_ENOREG,                  /*!< illegal register address. */
    MB_EINVAL,                  /*!< illegal argument. */
    MB_EPORTERR,                /*!< porting layer error. */
    MB_ENORES,                  /*!< insufficient resources. */
    MB_EIO,                     /*!< I/O error. */
    MB_EILLSTATE,               /*!< protocol stack in illegal state. */
    MB_ETIMEDOUT,               /*!< timeout error occurred. */
    MB_EDEVEXIST,
    MB_EILLNODEADDR,
    MB_ENODEEXIST,             /*!< node exist */
}MbErrorCode_t;
    
typedef struct
{
    uint16_t reg_holding_addr_start;
    uint16_t reg_input_addr_start;
    uint16_t reg_coils_addr_start;
    uint16_t reg_discrete_addr_start;
    uint16_t reg_holding_num;
    uint16_t reg_input_num;
    uint16_t reg_coils_num;
    uint16_t reg_discrete_num;
    uint16_t *pReghold;
    uint16_t *pReginput;
    uint8_t *pRegCoil;
    uint8_t *pRegDisc;
}MbReg_t;

typedef struct 
{
    uint16_t tid;
    uint16_t pid;
    uint16_t length;
    union {
        uint8_t uid;
        uint8_t slaveid;
    }introute;
}MbHeader_t;

typedef void (*pActionHandle)(void *dev);

/************************************** define for master ******************************************/
typedef enum
{
    MBR_ENOERR,
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
}MbReqResult_t;

typedef enum {
    MBM_IDLE,
    MBM_DELYPOLL,
    MBM_BROADCASTTURN,
    MBM_XMIT,
    MBM_XMITING,
    MBM_WAITRSP,
    MBM_RSPEXCUTE,
    MBM_RSPTIMEOUT
}MbmPollState_t;

typedef MbReqResult_t (*pMbmParseRspHandler)(MbReg_t *regs, 
                                                uint16_t ReqRegAddr, uint16_t ReqRegcnt, 
                                                uint8_t *premain,uint16_t remainLength);
typedef void (*pfnReqResultCB)(MbReqResult_t result, MbException_t eException, void *req);

typedef MbReqResult_t (*pActionMasterReceive)(void *pdev,MbHeader_t *phead,uint8_t *pfunCode, uint8_t **premain, uint16_t *premainLength);
typedef MbReqResult_t (*pActionMasterSend)(void *pdev,const uint8_t *pAdu, uint16_t usAduLength);

typedef struct
{
    uint8_t slaveaddr;      // slave address 
    uint8_t reserved0;      // reserved
    uint16_t reserved1;     // reserved
    MbReg_t regs;           // The register list
    pfnReqResultCB cb;      // call back function
    void *arg;              // arg for callback function
    void *next;
}MbmNode_t;

typedef struct
{
    MbmNode_t *node;        /* mark the node */
    uint32_t errcnt;        /* request error count*/
    uint8_t slaveaddr;      /* mark slave address */
    uint8_t funcode;        /* mark function code */
    uint16_t regaddr;       /* mark reg address for rsp used */
    uint16_t regcnt;        /* mark reg count for rsp used */
    uint16_t adulength;     /* mark adu length*/
    uint8_t *padu;          /* mark adu for repeat send */
    uint16_t scancnt;       /* scan time cnt */
    uint16_t scanrate;      /* scan rate  if 0 : once,  other request on scan rate */
    void *next;             /* request list */
}MbmReq_t;

typedef struct
{
    uint8_t port;               
    MbDevState_t devstate;      // device current state
    uint16_t reserved0;         //reserved
    
    MbmNode_t *nodehead;        /* slave node list on this host */

    MbmReq_t *Reqreadyhead;     /* request ready list  head*/
    MbmReq_t *Reqreadytail;     /* request ready list  tail*/
    MbmReq_t *Reqpendinghead;      /* request suspend list */

    MbMode_t currentMode;       // work mode as RTU ASCII TCP 

    uint8_t Pollstate;          // poll state

    uint8_t retry;              // retry MAX count
    uint8_t retrycnt;           // retry count

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
    
    void *next;
    
    /* low layer use */
    volatile uint8_t AsciiBytePos; // only for ascii
    volatile uint8_t sndrcvState;
    volatile uint16_t sndAduBufCount;
    volatile uint16_t sndAduBufPos;
    volatile uint16_t rcvAduBufPos;
    volatile uint8_t AduBuf[MB_ADU_SIZE_MAX];
}MbmDev_t;

#define MbmSetPollmode(dev,state) do {dev->Replytimeoutcnt = 0;dev->Pollstate = state;}while(0)
MbReqResult_t MbmSend(MbmDev_t *dev, MbmReq_t *req);

/************************************** define for slave ******************************************/
typedef MbException_t (*pMbsFunctionHandler)(MbReg_t *regs, uint8_t *pPdu, uint16_t *pusLength);

typedef MbErrorCode_t (*pActionSlaveReceive)(void *dev, uint8_t *pucRcvAddress, uint8_t **pPdu, uint16_t *pusLength);
typedef MbErrorCode_t (*pActionSlaveSend)(void *dev, uint8_t ucSlaveAddress, const uint8_t *pPdu, uint16_t usLength);

typedef struct
{
    uint16_t port;     
    uint8_t inuse;
    uint8_t reserved0;
    
    uint8_t slaveaddr;
    MbMode_t currentMode;    // work mode as RTU ASCII TCP?
    MbDevState_t devstate;   // device state
    uint8_t xEventInFlag;   // for event?
    
    MbReg_t regs;           // register list
    
    pActionHandle pMbStartCur;
    pActionHandle pMbStopCur;
    pActionHandle pMbCloseCur;
    pActionSlaveReceive pMbReceivedCur;
    pActionSlaveSend pMbSendCur;

    /* low layer use */
    volatile uint8_t AsciiBytePos; // only for ascii
    volatile uint8_t sndrcvState; 
    volatile uint16_t sndAduBufCount;
    volatile uint16_t sndAduBufPos;
    volatile uint16_t rcvAduBufPos;
    volatile uint8_t AduBuf[MB_ADU_SIZE_MAX];
}MbsDev_t;

#define xMBSemGive(dev) do { ((MbsDev_t *)dev)->xEventInFlag = TRUE;}while(0)


#endif
