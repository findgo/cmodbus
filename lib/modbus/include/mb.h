#ifndef __MB_H_
#define __MB_H_

#include "mbconfig.h"
#include "mbframe.h"
#include "mbdef.h" 

#define MBM_DEFAULT_RETRYCNT        0          /* default request failed to retry count */
#define MBM_RETRYCNT_MAX            10
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
}mb_Parity_t;

typedef void (*pActionHandle)(void *dev);

typedef mb_ErrorCode_t (*pActionReceive)(void *dev, uint8_t *pucRcvAddress, uint8_t **pPdu, uint16_t *pusLength);
typedef mb_ErrorCode_t (*pActionSend)(void *dev, uint8_t ucSlaveAddress, const uint8_t *pPdu, uint16_t usLength);

typedef struct
{
    uint16_t port; // ¶Ë¿ÚºÅ
    uint16_t reserved0;
    
    uint8_t slaveaddr;
    mb_Mode_t currentMode;    
    mb_DevState_t devstate;
    bool xEventInFlag; // for event?
    
    mb_Reg_t regs;
    
    pActionHandle pvMBStartCur;
    pActionHandle pvMBStopCur;
    pActionHandle pvMBCloseCur;
    pActionReceive peMBReceivedCur;
    pActionSend peMBSendCur;
#if MB_DYNAMIC_MEMORY_ALLOC_ENABLED > 0    
    void *next;
#endif
    /*  */
    volatile uint8_t AsciiBytePos; // only for ascii
    volatile uint8_t sndrcvState;
    volatile uint16_t sndAduBufCount;
    volatile uint16_t sndAduBufPos;
    volatile uint16_t rcvAduBufrPos;
    volatile uint8_t AduBuf[MB_ADU_SIZE_MAX];
}mb_Device_t;

#define xMBSemGive(dev) do { ((mb_Device_t *)dev)->xEventInFlag = true;}while(0)

/***************************************************/
/**************** define for master ********************/
/***************************************************/
typedef enum {
    MASTER_IDLE,
    MASTER_DELYPOLL,
    MASTER_BROADCASTTURN,
    MASTER_XMIT,
    MASTER_XMITING,
    MASTER_WAITRSP,
    MASTER_RSPEXCUTE,
    MASTER_RSPTIMEOUT
}Master_Pollstate_t;

typedef mb_reqresult_t (*pActionMasterReceive)(void *pdev,mb_header_t *phead,uint8_t *pfunCode, uint8_t **premain, uint16_t *premainLength);
typedef mb_reqresult_t (*pActionMasterSend)(void *pdev,const uint8_t *pAdu, uint16_t usAduLength);

typedef struct
{
    uint8_t slaveaddr;
    uint8_t reserved0;
    uint16_t reserved1;
    mb_Reg_t regs;
    void *next;
}mb_slavenode_t;

typedef struct
{
    mb_slavenode_t *node;   /* mark the node */
    uint32_t errcnt;        /* request errcnt */
    uint8_t slaveaddr;      /* mark slave address */
    uint8_t funcode;        /* mark function code */
    uint16_t regaddr;       /* mark reg address for rsp used */
    uint16_t regcnt;        /* mark reg count for rsp used */
    uint16_t adulength;     /* mark adu length*/
    uint8_t *padu;          /* mark adu for repeat send */
    uint16_t scancnt;       /* scan time cnt */
    uint16_t scanrate;      /* scan rate  if 0 : once,other request on scan rate */
    pReqResultCB cb;
    void *next;
}mb_request_t;

typedef struct
{
    uint8_t port; // ¶Ë¿ÚºÅ
    mb_DevState_t devstate;
    uint16_t reserved0;
    
    mb_slavenode_t *nodehead;   /* slave node list on this host */

    mb_request_t *Reqreadyhead; /* request ready list  head*/
    mb_request_t *Reqreadytail; /* request ready list  tail*/
    mb_request_t *Reqpendhead;  /* request suspend list */

    mb_Mode_t currentMode;    

    uint8_t Pollstate;

    uint8_t retry;
    uint8_t retrycnt;

    uint16_t Replytimeout;              /* response timeout */
    uint16_t Replytimeoutcnt;           /* response timeout count*/   
    uint16_t Delaypolltime;             /* delay time between polls */
    uint16_t Delaypolltimecnt;          /* delay time between polls count*/
    uint16_t Broadcastturntime;         /* after broadcast turn round time */
    uint16_t Broadcastturntimecnt;      /* after broadcast turn round time count*/
    
    pActionHandle pvMBStartCur;
    pActionHandle pvMBStopCur;
    pActionHandle pvMBCloseCur;
    pActionMasterSend peMBSendCur;
    pActionMasterReceive peMBReceivedCur;
    
    void *next;

    volatile uint8_t AsciiBytePos; // only for ascii
    volatile uint8_t sndrcvState;
    volatile uint16_t sndAduBufCount;
    volatile uint16_t sndAduBufPos;
    volatile uint16_t rcvAduBufrPos;
    volatile uint8_t AduBuf[MB_ADU_SIZE_MAX];
}mb_MasterDevice_t;

#define vMBMasterSetPollmode(dev,state) do {dev->Replytimeoutcnt = 0;dev->Pollstate = state;}while(0)

mb_reqresult_t eMBMaster_Reqsend(mb_MasterDevice_t *dev, mb_request_t *req);


#endif
