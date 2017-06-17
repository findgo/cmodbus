
#ifndef __MODBUS_H_
#define __MODBUS_H_

#include "mbconfig.h"
#include "mbframe.h"
#include "mbproto.h"
#include "mbcpu.h"

#include "port.h"

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
} eMBMode;
    
typedef enum
{
    DEV_STATE_NOT_INITIALIZED,
    DEV_STATE_DISABLED,
    DEV_STATE_ENABLED
}eMBDevState;
    
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
    MB_ECHANNELEXIST,            /*!< channel exist. */
    MB_EINCHANNEL               /*!< illegal channel */
}eMBErrorCode;

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
}mb_reg_t;

typedef eMBException (*pxMBFunctionHandler)(mb_reg_t *regs, uint8_t *pPdu, uint16_t *pusLength);
typedef void (*pActionHandle)(void *dev);
typedef eMBErrorCode (*pActionReceive)(void *dev, uint8_t *pucRcvAddress, uint8_t **pPdu, uint16_t *pusLength);
typedef eMBErrorCode (*pActionSend)(void *dev, uint8_t ucSlaveAddress, const uint8_t *pPdu, uint16_t usLength);

typedef struct
{
    uint16_t port; // ¶Ë¿ÚºÅ
    uint8_t slaveid;
    eMBMode currentMode;
    
    eMBDevState devstate;
    
    bool xEventInFlag; // for event?
    uint8_t reserved0;
    
    volatile uint8_t AsciiBytePos; // only for ascii
    
    mb_reg_t regs;
    
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
}mb_device_t;

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
 *   mbconfig.h is to small it returns eMBErrorCode::MB_ENORES. Otherwise
 *   it returns eMBErrorCode::MB_ENOERR.
 */
eMBErrorCode eMBSetSlaveID(mb_reg_t *regs, uint8_t ucSlaveID, bool xIsRunning,
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
 * \return eMBErrorCode::MB_ENOERR if the handler has been installed. If no
 *   more resources are available it returns eMBErrorCode::MB_ENORES. In this
 *   case the values in mbconfig.h should be adjusted. If the argument was not
 *   valid it returns eMBErrorCode::MB_EINVAL.
 */
eMBErrorCode eMBRegisterCB( uint8_t ucFunctionCode, pxMBFunctionHandler pxHandler );
eMBErrorCode eMBOpen(mb_device_t *dev, uint8_t channel, eMBMode eMode, uint8_t ucSlaveAddress, 
                        uint8_t ucPort, uint32_t ulBaudRate, eMBParity eParity );
eMBErrorCode eMBTCPOpen(mb_device_t *dev,uint8_t channel, uint16_t ucTCPPort);
eMBErrorCode eMBClose(mb_device_t *dev);
eMBErrorCode eMBStart(mb_device_t *dev);
eMBErrorCode eMBStop(mb_device_t *dev);
void vMBPoll(void);
eMBErrorCode eMBRegCreate(mb_device_t *dev,
                            uint8_t *regbuf,
                            uint16_t reg_holding_addr_start,
                            uint16_t reg_holding_num,
                            uint16_t reg_input_addr_start,
                            uint16_t reg_input_num,
                            uint16_t reg_coils_addr_start,
                            uint16_t reg_coils_num,
                            uint16_t reg_discrete_addr_start,
                            uint16_t reg_discrete_num);

#endif

