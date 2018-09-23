#ifndef __MB_H_
#define __MB_H_

#include "mbconfig.h"
#include "mbdef.h" 

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

enum 
{
    MBCOM0 = 0,
    MBCOM1,
    MBCOM2,
    MBCOM3
};

/*! \ingroup modbus
 * \brief Parity used for characters in serial mode.
 *
 * The parity which should be applied to the characters sent over the serial
 * link. Please note that this values are actually passed to the porting
 * layer and therefore not all parity modes might be available.
 */
typedef enum
{
    MB_PAR_NONE,                /*!< No parity. */
    MB_PAR_ODD,                 /*!< Odd parity. */
    MB_PAR_EVEN                 /*!< Even parity. */
}MbParity_t;

typedef void (*pActionHandle)(void *dev);

typedef MbErrorCode_t (*pActionReceive)(void *dev, uint8_t *pucRcvAddress, uint8_t **pPdu, uint16_t *pusLength);
typedef MbErrorCode_t (*pActionSend)(void *dev, uint8_t ucSlaveAddress, const uint8_t *pPdu, uint16_t usLength);

typedef struct
{
    uint16_t port; 
    uint8_t inuse;
    uint8_t reserved0;
    
    uint8_t slaveaddr;
    MbMode_t currentMode;    
    MbDevState_t devstate;
    uint8_t xEventInFlag; // for event?
    // register list
    MbReg_t regs;
    
    pActionHandle pMbStartCur;
    pActionHandle pMbStopCur;
    pActionHandle pMbCloseCur;
    pActionReceive pMbReceivedCur;
    pActionSend pMbSendCur;

    /*  */
    volatile uint8_t AsciiBytePos; // only for ascii
    volatile uint8_t sndrcvState;
    volatile uint16_t sndAduBufCount;
    volatile uint16_t sndAduBufPos;
    volatile uint16_t rcvAduBufPos;
    volatile uint8_t AduBuf[MB_ADU_SIZE_MAX];
}MbsDevice_t;

#define xMBSemGive(dev) do { ((MbsDevice_t *)dev)->xEventInFlag = TRUE;}while(0)

/***************************************************/
/**************** define for master ********************/
/***************************************************/
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

typedef MbReqResult_t (*pActionMasterReceive)(void *pdev,MbHeader_t *phead,uint8_t *pfunCode, uint8_t **premain, uint16_t *premainLength);
typedef MbReqResult_t (*pActionMasterSend)(void *pdev,const uint8_t *pAdu, uint16_t usAduLength);

typedef struct
{
    uint8_t slaveaddr;
    uint8_t reserved0;
    uint16_t reserved1;
    MbReg_t regs;
    pfnReqResultCB cb;        // call back function
    void *arg;              // arg for callback function
    void *next;
}MbmNode_t;

typedef struct
{
    MbmNode_t *node;   /* mark the node */
    uint32_t errcnt;        /* request errcnt */
    uint8_t slaveaddr;      /* mark slave address */
    uint8_t funcode;        /* mark function code */
    uint16_t regaddr;       /* mark reg address for rsp used */
    uint16_t regcnt;        /* mark reg count for rsp used */
    uint16_t adulength;     /* mark adu length*/
    uint8_t *padu;          /* mark adu for repeat send */
    uint16_t scancnt;       /* scan time cnt */
    uint16_t scanrate;      /* scan rate  if 0 : once,other request on scan rate */
    void *next;
}MbmReq_t;

typedef struct
{
    uint8_t port; // 
    MbDevState_t devstate;
    uint16_t reserved0;
    
    MbmNode_t *nodehead;   /* slave node list on this host */

    MbmReq_t *Reqreadyhead; /* request ready list  head*/
    MbmReq_t *Reqreadytail; /* request ready list  tail*/
    MbmReq_t *Reqpendhead;  /* request suspend list */

    MbMode_t currentMode;    

    uint8_t Pollstate;

    uint8_t retry;
    uint8_t retrycnt;

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

    volatile uint8_t AsciiBytePos; // only for ascii
    volatile uint8_t sndrcvState;
    volatile uint16_t sndAduBufCount;
    volatile uint16_t sndAduBufPos;
    volatile uint16_t rcvAduBufPos;
    volatile uint8_t AduBuf[MB_ADU_SIZE_MAX];
}MbmDev_t;

#define MbmSetPollmode(dev,state) do {dev->Replytimeoutcnt = 0;dev->Pollstate = state;}while(0)

MbReqResult_t MbmSend(MbmDev_t *dev, MbmReq_t *req);


#endif
