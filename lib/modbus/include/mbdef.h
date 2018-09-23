#ifndef __MBDEF_H_
#define __MBDEF_H_

#include "mbproto.h"
#include "mbcpu.h"


#ifndef FALSE
#define FALSE   0
#endif

#ifndef TRUE
#define TRUE   1
#endif


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
} MbMode_t;

typedef enum
{
    DEV_STATE_NOT_INITIALIZED,
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

typedef MbException_t (*pMbsFunctionHandler)(MbReg_t *regs, uint8_t *pPdu, uint16_t *pusLength);

typedef MbReqResult_t (*pMbmParseRspHandler)(MbReg_t *regs, 
                                            uint16_t ReqRegAddr, uint16_t ReqRegcnt, 
                                            uint8_t *premain,uint16_t remainLength);

typedef void (*pfnReqResultCB)(MbReqResult_t result,MbException_t eException, void *req);


#endif

