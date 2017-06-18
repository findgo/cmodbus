
#ifndef __MODBUS_H_
#define __MODBUS_H_

#include "mbconfig.h"
#include "mbframe.h"
#include "mbproto.h"
#include "mbcpu.h"

#include "port.h"


#if MB_DYNAMIC_MEMORY_ALLOC_ENABLE > 0
#define mb_malloc malloc
#define mb_free free
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
} mb_Mode_t;
    
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
    MB_EDEVEXIST                /*!< device exist */
}mb_ErrorCode_t;

typedef struct
{
    uint16_t reg_coils_addr_start;
    uint16_t reg_discrete_addr_start;
    uint16_t reg_holding_addr_start;
    uint16_t reg_input_addr_start;
    uint16_t reg_coils_num;
    uint16_t reg_discrete_num;
    uint16_t reg_holding_num;
    uint16_t reg_input_num;
    uint8_t *pRegCoil;
    uint8_t *pRegDisc;
    uint16_t *pReghold;
    uint16_t *pReginput;
}mb_Reg_t;

typedef eMBException_t (*pxMBFunctionHandler)(mb_Reg_t *regs, uint8_t *pPdu, uint16_t *pusLength);
typedef void (*pActionHandle)(void *dev);
typedef mb_ErrorCode_t (*pActionReceive)(void *dev, uint8_t *pucRcvAddress, uint8_t **pPdu, uint16_t *pusLength);
typedef mb_ErrorCode_t (*pActionSend)(void *dev, uint8_t ucSlaveAddress, const uint8_t *pPdu, uint16_t usLength);

typedef struct
{
    uint16_t port; // ¶Ë¿ÚºÅ
    uint8_t slaveid;
    mb_Mode_t currentMode;
    
    mb_DevState_t devstate;
    
    bool xEventInFlag; // for event?
    uint8_t reserved0;
    
    volatile uint8_t AsciiBytePos; // only for ascii
    
    mb_Reg_t regs;
    
    pActionHandle pvMBStartCur;
    pActionHandle pvMBStopCur;
    pActionHandle pvMBCloseCur;
    pActionReceive peMBReceivedCur;
    pActionSend peMBSendCur;
    
    void *next;
    
    volatile uint8_t sndState;
    volatile uint8_t rcvState;
    volatile uint16_t sndAduBufCount;
    volatile uint16_t sndAduBufPos;
    volatile uint16_t rcvAduBufrPos;
    volatile uint8_t AduBuf[MB_ADU_SIZE_MAX];
}mb_Device_t;

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
                        uint8_t ucPort, uint32_t ulBaudRate, mb_Parity_t eParity );
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


#endif

