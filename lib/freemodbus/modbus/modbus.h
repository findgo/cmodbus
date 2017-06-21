
#ifndef __MODBUS_H_
#define __MODBUS_H_

#include "mbconfig.h"
#include "mbframe.h"
#include "mbproto.h"
#include "mbcpu.h"


#if MB_DYNAMIC_MEMORY_ALLOC_ENABLE > 0
#define mb_malloc malloc
#define mb_free free
#endif


#define MBM_DEFAULT_RETRYCNT        0          /* default request failed to retry count */
#define MBM_RETRYCNT_MAX            10
/* for master defined (units of millisecond)*/
#define MBM_DEFAULT_REPLYTIMEOUT    1000     /* response timeout */
#define MBM_DEFAULT_DELAYPOLLTIME   50         /* delay time between polls */
#define MBM_DEFAULT_BROADTURNTIME   100        /* after broadcast turn round time */
#define MBM_DEFAULT_SCANRATE        1000        /* every request scan rate */

#define MBM_REPLYTIMEOUT_MIN    50         /* response timeout min*/
#define MBM_REPLYTIMEOUT_MAX    60000     /* response timeout max*/
#define MBM_DELAYPOLLTIME_MIN   50         /* delay time between polls */
#define MBM_DELAYPOLLTIME_MAX   200        /* delay time between polls */
#define MBM_BROADTURNTIME_MIN   50         /* after broadcast turn round time */
#define MBM_BROADTURNTIME_MAX   200        /* after broadcast turn round time */
#define MBM_SCANRATE_MAX        60000       /* every request scan rate */

/*! \ingroup modbus
 * \brief Modbus serial transmission modes (RTU/ASCII).
 *
 * Modbus serial supports two transmission modes. Either ASCII or RTU. RTU
 * is faster but has more hardware requirements and requires a network with
 * a low jitter. ASCII is slower and more reliable on slower links (E.g. modems)
 */
typedef enum
{
    MB_RTU,     /*!< RTU transmission mode. */
    MB_ASCII,   /*!< ASCII transmission mode. */
    MB_TCP      /*!< TCP mode. */
} mb_Mode_t;
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
    
typedef enum
{
    DEV_STATE_NOT_INITIALIZED,
    DEV_STATE_DISABLED,
    DEV_STATE_ENABLED
}mb_DevState_t;
    
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
    // for master
    MBM_ENODEEXIST,             /*!< node exist */
    MBM_EINNODEADDR,            /* illegal slave address*/
    MBM_ENODENOSETUP,           /* node not yet established */
    MBM_ENOMEM,                 /* Out of memory */
}mb_ErrorCode_t;

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
}mb_Reg_t;

typedef void (*pActionHandle)(void *dev);


typedef eMBException_t (*pxMBFunctionHandler)(mb_Reg_t *regs, uint8_t *pPdu, uint16_t *pusLength);
typedef mb_ErrorCode_t (*pActionReceive)(void *dev, uint8_t *pucRcvAddress, uint8_t **pPdu, uint16_t *pusLength);
typedef mb_ErrorCode_t (*pActionSend)(void *dev, uint8_t ucSlaveAddress, const uint8_t *pPdu, uint16_t usLength);

typedef struct
{
    uint16_t port; // ¶Ë¿ÚºÅ
    uint8_t slaveaddr;
    mb_Mode_t currentMode;
    
    mb_DevState_t devstate;
        
    volatile uint8_t AsciiBytePos; // only for ascii
    uint16_t reserved0;
    
    mb_Reg_t regs;
    
    pActionHandle pvMBStartCur;
    pActionHandle pvMBStopCur;
    pActionHandle pvMBCloseCur;
    pActionReceive peMBReceivedCur;
    pActionSend peMBSendCur;
    
    void *next;
    /*  */
    bool xEventInFlag; // for event?
    volatile uint8_t sndrcvState;
    volatile uint16_t sndAduBufCount;
    volatile uint16_t sndAduBufPos;
    volatile uint16_t rcvAduBufrPos;
    volatile uint8_t AduBuf[MB_ADU_SIZE_MAX];
}mb_Device_t;

/**************** define for master *******************/
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


typedef struct 
{
    uint16_t tid;
    uint16_t pid;
    uint16_t length;
    union {
        uint8_t uid;
        uint8_t slaveid;
    }introute;
}mb_header_t;

typedef mb_ErrorCode_t (*pxMBParseRspHandler)(mb_Reg_t *regs, 
                                    uint16_t RegStartAddr, uint16_t regcnt, 
                                    uint8_t *premain,uint16_t remainLength);

typedef mb_ErrorCode_t (*pActionMasterReceive)(void *pdev,mb_header_t *phead,uint8_t *pfunCode, uint8_t **premain, uint16_t *premainLength);
typedef mb_ErrorCode_t (*pActionMasterSend)(void *pdev,const uint8_t *pAdu, uint16_t usAduLength);



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
    uint32_t errcnt;    /* request errcnt */
    uint8_t slaveaddr;      /* mark slave address */
    uint8_t funcode;        /* mark function code */
    uint16_t regaddr;       /* mark reg address for rsp used */
    uint16_t regcnt;        /* mark reg count for rsp used */
    uint16_t adulength;     /* mark adu length*/
    uint8_t *padu;          /* mark adu for repeat send */
    uint16_t scancnt;       /* scan time cnt */
    uint16_t scanrate;      /* scan rate  if 0 : once,other request on scan rate */
    //void *ReqSuccessCB(void *req);
    void *next;
}mb_request_t;

typedef struct
{
    uint16_t port; // ¶Ë¿ÚºÅ
    mb_Mode_t currentMode;
    
    mb_DevState_t devstate;
    
    mb_slavenode_t *nodehead; /* slave node list on this host */

    mb_request_t *Reqreadyhead; /* request ready list  head*/
    mb_request_t *Reqreadytail; /* request ready list  tail*/
    mb_request_t *Reqpendhead;  /* request suspend list */
    
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
    pActionMasterReceive peMBReceivedCur;
    pActionMasterSend peMBSendCur;
    
    void *next;

    volatile uint8_t AsciiBytePos; // only for ascii
    volatile uint8_t sndrcvState;
    volatile uint16_t sndAduBufCount;
    volatile uint16_t sndAduBufPos;
    volatile uint16_t rcvAduBufrPos;
    volatile uint8_t AduBuf[MB_ADU_SIZE_MAX];
}mb_MasterDevice_t;



/*! \ingroup modbus
 * \brief Configure the slave id of the device.
 *
 * This function should be called when the Modbus function <em>Report Slave ID</em>
 * is enabled ( By defining MB_FUNC_OTHER_REP_SLAVEID_ENABLED in mbconfig.h ).
 *
 * \param ucSlaveID Values is returned in the <em>Slave ID</em> byte of the
 *   <em>Report Slave ID</em> response.
 * \param xIsRunning If true the <em>Run Indicator Status</em> byte is set to 0xFF.
 *   otherwise the <em>Run Indicator Status</em> is 0x00.
 * \param pucAdditional Values which should be returned in the <em>Additional</em>
 *   bytes of the <em> Report Slave ID</em> response.
 * \param usAdditionalLen Length of the buffer <code>pucAdditonal</code>.
 *
 * \return If the static buffer defined by MB_FUNC_OTHER_REP_SLAVEID_BUF in
 *   mbconfig.h is to small it returns mb_ErrorCode_t::MB_ENORES. Otherwise
 *   it returns mb_ErrorCode_t::MB_ENOERR.
 */
mb_ErrorCode_t eMBSetSlaveID(mb_Reg_t *regs, uint8_t ucSlaveID, bool xIsRunning,
                uint8_t const *pucAdditional, uint16_t usAdditionalLen );

/*! \ingroup modbus
 * \brief Registers a callback handler for a given function code.
 *
 * This function registers a new callback handler for a given function code.
 * The callback handler supplied is responsible for interpreting the Modbus PDU and
 * the creation of an appropriate response. In case of an error it should return
 * one of the possible Modbus exceptions which results in a Modbus exception frame
 * sent by the protocol stack. 
 *
 * \param ucFunctionCode The Modbus function code for which this handler should
 *   be registers. Valid function codes are in the range 1 to 127.
 * \param pxHandler The function handler which should be called in case
 *   such a frame is received. If \c NULL a previously registered function handler
 *   for this function code is removed.
 *
 * \return mb_ErrorCode_t::MB_ENOERR if the handler has been installed. If no
 *   more resources are available it returns mb_ErrorCode_t::MB_ENORES. In this
 *   case the values in mbconfig.h should be adjusted. If the argument was not
 *   valid it returns mb_ErrorCode_t::MB_EINVAL.
 */
mb_ErrorCode_t eMBRegisterCB(uint8_t ucFunctionCode, pxMBFunctionHandler pxHandler);
#if MB_DYNAMIC_MEMORY_ALLOC_ENABLE > 0
uint32_t xMBRegBufSizeCal(     uint16_t reg_holding_num,
                            uint16_t reg_input_num,
                            uint16_t reg_coils_num,
                            uint16_t reg_discrete_num);
mb_Device_t *xMBBaseDeviceNew(void);
uint8_t *xMBRegBufNew(uint32_t size);
#endif
mb_ErrorCode_t eMBOpen(mb_Device_t *dev, mb_Mode_t eMode, uint8_t ucSlaveAddress, 
                        uint8_t ucPort, uint32_t ulBaudRate, mb_Parity_t eParity);
mb_ErrorCode_t eMBTCPOpen(mb_Device_t *dev, uint16_t ucTCPPort);
mb_ErrorCode_t eMBRegCreate(mb_Device_t *dev,
                            uint8_t *regbuf,
                            uint16_t reg_holding_addr_start,
                            uint16_t reg_holding_num,
                            uint16_t reg_input_addr_start,
                            uint16_t reg_input_num,
                            uint16_t reg_coils_addr_start,
                            uint16_t reg_coils_num,
                            uint16_t reg_discrete_addr_start,
                            uint16_t reg_discrete_num);
mb_ErrorCode_t eMBStart(mb_Device_t *dev);
mb_ErrorCode_t eMBStop(mb_Device_t *dev);
mb_ErrorCode_t eMBClose(mb_Device_t *dev);
void vMBPoll(void);


/* TODO implement modbus master */




#endif

