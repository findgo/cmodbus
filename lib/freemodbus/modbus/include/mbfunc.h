
#ifndef __MB_FUNC_H
#define __MB_FUNC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mbconfig.h"
#include "mbproto.h"
#include "mbframe.h"
#include "mbcpu.h"

#include "mbutils.h"
#include "mbbuf.h"
#include "mbfunc.h"
#include "modbus.h"
    
// read holding input coil disc offset in pdu
#define MB_PDU_FUNC_READ_ADDR_OFF           ( MB_PDU_DATA_OFF)
#define MB_PDU_FUNC_READ_REGCNT_OFF         ( MB_PDU_DATA_OFF + 2 ) // only for holding reg
#define MB_PDU_FUNC_READ_BITSCNT_OFF        ( MB_PDU_DATA_OFF + 2 ) // only for coils
#define MB_PDU_FUNC_READ_SIZE               ( 4 )

// write single holding coil offset in pdu
#define MB_PDU_FUNC_WRITE_ADDR_OFF          ( MB_PDU_DATA_OFF )
#define MB_PDU_FUNC_WRITE_VALUE_OFF         ( MB_PDU_DATA_OFF + 2 )
#define MB_PDU_FUNC_WRITE_SIZE              ( 4 )

// write multiple holding coils offset in pdu
#define MB_PDU_FUNC_WRITE_MUL_ADDR_OFF      ( MB_PDU_DATA_OFF )
#define MB_PDU_FUNC_WRITE_MUL_REGCNT_OFF    ( MB_PDU_DATA_OFF + 2 ) // for holding reg
#define MB_PDU_FUNC_WRITE_MUL_COILCNT_OFF   ( MB_PDU_DATA_OFF + 2 ) // for coils
#define MB_PDU_FUNC_WRITE_MUL_BYTECNT_OFF   ( MB_PDU_DATA_OFF + 4 )
#define MB_PDU_FUNC_WRITE_MUL_VALUES_OFF    ( MB_PDU_DATA_OFF + 5 )
#define MB_PDU_FUNC_WRITE_MUL_SIZE_MIN      ( 5 )

// readwrite multiple holding offset in pdu
#define MB_PDU_FUNC_READWRITE_READ_ADDR_OFF     ( MB_PDU_DATA_OFF + 0 )
#define MB_PDU_FUNC_READWRITE_READ_REGCNT_OFF   ( MB_PDU_DATA_OFF + 2 )
#define MB_PDU_FUNC_READWRITE_WRITE_ADDR_OFF    ( MB_PDU_DATA_OFF + 4 )
#define MB_PDU_FUNC_READWRITE_WRITE_REGCNT_OFF  ( MB_PDU_DATA_OFF + 6 )
#define MB_PDU_FUNC_READWRITE_BYTECNT_OFF       ( MB_PDU_DATA_OFF + 8 )
#define MB_PDU_FUNC_READWRITE_WRITE_VALUES_OFF  ( MB_PDU_DATA_OFF + 9 )
#define MB_PDU_FUNC_READWRITE_SIZE_MIN          ( 9 )

/*! \ingroup modbus
 * \brief If register should be written or read.
 *
 * This value is passed to the callback functions which support either
 * reading or writing register values. Writing means that the application
 * registers should be updated and reading means that the modbus protocol
 * stack needs to know the current register values.
 *
 * \see eMBRegHoldingCB( ), eMBRegCoilsCB( ), eMBRegDiscreteCB( ) and 
 *   eMBRegInputCB( ).
 */
typedef enum
{
    MB_REG_READ,                /*!< Read register values and pass to protocol stack. */
    MB_REG_WRITE                /*!< Update register values. */
} mb_RegisterMode_t;

#if MB_FUNC_OTHER_REP_SLAVEID_BUF > 0
eMBException_t eMBFuncReportSlaveID(mb_Reg_t *regs, uint8_t *pPdu, uint16_t * usLen);
#endif

#if MB_FUNC_READ_INPUT_ENABLED > 0
eMBException_t eMBFuncRdInputRegister(mb_Reg_t *regs, uint8_t *pPdu, uint16_t * usLen);
#endif

#if MB_FUNC_READ_HOLDING_ENABLED > 0
eMBException_t eMBFuncRdHoldingRegister(mb_Reg_t *regs, uint8_t *pPdu, uint16_t * usLen);
#endif

#if MB_FUNC_WRITE_HOLDING_ENABLED > 0
eMBException_t eMBFuncWrHoldingRegister(mb_Reg_t *regs, uint8_t *pPdu, uint16_t * usLen);
#endif

#if MB_FUNC_WRITE_MULTIPLE_HOLDING_ENABLED > 0
eMBException_t eMBFuncWrMulHoldingRegister(mb_Reg_t *regs, uint8_t *pPdu, uint16_t *usLen);
#endif

#if MB_FUNC_READ_COILS_ENABLED > 0
eMBException_t eMBFuncRdCoils(mb_Reg_t *regs, uint8_t *pPdu, uint16_t *usLen );
#endif

#if MB_FUNC_WRITE_COIL_ENABLED > 0
eMBException_t eMBFuncWrCoil(mb_Reg_t *regs, uint8_t *pPdu, uint16_t *usLen);
#endif

#if MB_FUNC_WRITE_MULTIPLE_COILS_ENABLED > 0
eMBException_t eMBFuncWrMulCoils(mb_Reg_t *regs, uint8_t *pPdu, uint16_t *usLen);
#endif

#if MB_FUNC_READ_DISCRETE_INPUTS_ENABLED > 0
eMBException_t eMBFuncRdDiscreteInputs(mb_Reg_t *regs, uint8_t *pPdu, uint16_t *usLen);
#endif

#if MB_FUNC_READWRITE_HOLDING_ENABLED > 0
eMBException_t eMBFuncRdWrMulHoldingRegister(mb_Reg_t *regs, uint8_t *pPdu, uint16_t *usLen);
#endif


/* for bits */
mb_ErrorCode_t eMBReqRdCoils(mb_MasterDevice_t *Mdev, uint8_t slaveaddr, 
                                uint16_t RegStartAddr, uint16_t Coilcnt, uint16_t scanrate);
mb_ErrorCode_t eMBReqWrCoil(mb_MasterDevice_t *Mdev, uint8_t slaveaddr, 
                                uint16_t RegAddr, uint16_t val);
mb_ErrorCode_t eMbReqWrMulCoils(mb_MasterDevice_t *Mdev, uint8_t slaveaddr, 
                                        uint16_t RegStartAddr, uint16_t Coilcnt,
                                        uint8_t *valbuf, uint16_t valcnt);
mb_ErrorCode_t eMBReqRdDiscreteInputs(mb_MasterDevice_t *Mdev, uint8_t slaveaddr, 
                                        uint16_t RegStartAddr, uint16_t Discnt, uint16_t scanrate);

mb_ErrorCode_t eMBParseRspRdCoils(mb_Reg_t *regs, 
                                    uint16_t RegStartAddr, uint16_t Coilcnt, 
                                    uint8_t *premain,uint16_t remainLength);
mb_ErrorCode_t eMBParseRspWrCoil(mb_Reg_t *regs, 
                                    uint16_t RegAddr, uint16_t Coilcnt,
                                    uint8_t *premain, uint16_t remainLength);
mb_ErrorCode_t eMBParseRspWrMulCoils(mb_Reg_t *regs, 
                                    uint16_t RegStartAddr, uint16_t Coilcnt,
                                    uint8_t *premain, uint16_t remainLength);
mb_ErrorCode_t eMBParseRspRdDiscreteInputs(mb_Reg_t *regs, 
                                    uint16_t RegStartAddr, uint16_t Discnt, 
                                    uint8_t *premain, uint16_t remainLength);
/* for register */
mb_ErrorCode_t eMBReqRdHoldingRegister(mb_MasterDevice_t *Mdev, uint8_t slaveaddr, 
                                        uint16_t RegStartAddr, uint16_t Regcnt, uint16_t scanrate);
mb_ErrorCode_t eMBReqWrHoldingRegister(mb_MasterDevice_t *Mdev, uint8_t slaveaddr, 
                                        uint16_t RegAddr, uint16_t val);
mb_ErrorCode_t eMbReqWrMulHoldingRegister(mb_MasterDevice_t *Mdev, uint8_t slaveaddr, 
                                        uint16_t RegStartAddr, uint16_t Regcnt,
                                        uint16_t *valbuf, uint16_t valcnt);
mb_ErrorCode_t eMBReqRdInputRegister(mb_MasterDevice_t *Mdev, uint8_t slaveaddr, 
                                        uint16_t RegStartAddr, uint16_t Regcnt, uint16_t scanrate);

mb_ErrorCode_t eMBReqRdWrMulHoldingRegister(mb_MasterDevice_t *Mdev, uint8_t slaveaddr, 
                                                    uint16_t RegReadStartAddr, uint16_t RegReadCnt,
                                                    uint16_t RegWriteStartAddr, uint16_t RegWriteCnt,
                                                    uint16_t *valbuf, uint16_t valNUM);
mb_ErrorCode_t eMBParseRspRdHoldingRegister(mb_Reg_t *regs, 
                                                uint16_t RegStartAddr, uint16_t Regcnt,
                                                uint8_t *premain, uint16_t remainLength);
mb_ErrorCode_t eMBParseRspWrHoldingRegister(mb_Reg_t *regs, 
                                                    uint16_t RegAddr, uint16_t Regcnt,
                                                    uint8_t *premain, uint16_t remainLength);
mb_ErrorCode_t eMBParseRspWrMulHoldingRegister(mb_Reg_t *regs, 
                                                        uint16_t RegStartAddr,uint16_t Regcnt, 
                                                        uint8_t *premain, uint16_t remainLength);
mb_ErrorCode_t eMBParseRspRdWrMulHoldingRegister(mb_Reg_t *regs, 
                                                        uint16_t RegStartAddr,uint16_t Regcnt, 
                                                        uint8_t *premain, uint16_t remainLength);
mb_ErrorCode_t eMBParseRdInputRegister(mb_Reg_t *regs, 
                                            uint16_t RegStartAddr, uint16_t Regcnt,
                                            uint8_t *premain, uint16_t remainLength);
    
#ifdef __cplusplus
}
#endif
#endif

