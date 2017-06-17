
#ifndef _MB_FUNC_H
#define _MB_FUNC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mbconfig.h"
#include "mbproto.h"
#include "mbframe.h"
#include "mbcpu.h"

#include "modbus.h"
#include "mbutils.h"
#include "mbfunccb.h"

// read holding input coil disc 
#define MB_PDU_FUNC_READ_SIZE               ( 4 )
#define MB_PDU_FUNC_READ_ADDR_OFF           ( MB_PDU_DATA_OFF)
#define MB_PDU_FUNC_READ_REGCNT_OFF         ( MB_PDU_DATA_OFF + 2 ) // only for holding reg
#define MB_PDU_FUNC_READ_BITSCNT_OFF        ( MB_PDU_DATA_OFF + 2 ) // only for coils

// write single holding coil 
#define MB_PDU_FUNC_WRITE_ADDR_OFF          ( MB_PDU_DATA_OFF )
#define MB_PDU_FUNC_WRITE_VALUE_OFF         ( MB_PDU_DATA_OFF + 2 )
#define MB_PDU_FUNC_WRITE_SIZE              ( 4 )

// write multiple holding coils
#define MB_PDU_FUNC_WRITE_MUL_ADDR_OFF      ( MB_PDU_DATA_OFF )
#define MB_PDU_FUNC_WRITE_MUL_REGCNT_OFF    ( MB_PDU_DATA_OFF + 2 ) // for holding reg
#define MB_PDU_FUNC_WRITE_MUL_COILCNT_OFF   ( MB_PDU_DATA_OFF + 2 ) // for coils
#define MB_PDU_FUNC_WRITE_MUL_BYTECNT_OFF   ( MB_PDU_DATA_OFF + 4 )
#define MB_PDU_FUNC_WRITE_MUL_VALUES_OFF    ( MB_PDU_DATA_OFF + 5 )
#define MB_PDU_FUNC_WRITE_MUL_SIZE_MIN      ( 5 )

// readwrite multiple holding
#define MB_PDU_FUNC_READWRITE_READ_ADDR_OFF     ( MB_PDU_DATA_OFF + 0 )
#define MB_PDU_FUNC_READWRITE_READ_REGCNT_OFF   ( MB_PDU_DATA_OFF + 2 )
#define MB_PDU_FUNC_READWRITE_WRITE_ADDR_OFF    ( MB_PDU_DATA_OFF + 4 )
#define MB_PDU_FUNC_READWRITE_WRITE_REGCNT_OFF  ( MB_PDU_DATA_OFF + 6 )
#define MB_PDU_FUNC_READWRITE_BYTECNT_OFF       ( MB_PDU_DATA_OFF + 8 )
#define MB_PDU_FUNC_READWRITE_WRITE_VALUES_OFF  ( MB_PDU_DATA_OFF + 9 )
#define MB_PDU_FUNC_READWRITE_SIZE_MIN          ( 9 )

#if MB_FUNC_OTHER_REP_SLAVEID_BUF > 0
eMBException eMBFuncReportSlaveID(mb_reg_t *regs, uint8_t *pPdu, uint16_t * usLen);
#endif

#if MB_FUNC_READ_INPUT_ENABLED > 0
eMBException eMBFuncReadInputRegister(mb_reg_t *regs, uint8_t *pPdu, uint16_t * usLen);
#endif

#if MB_FUNC_READ_HOLDING_ENABLED > 0
eMBException eMBFuncReadHoldingRegister(mb_reg_t *regs, uint8_t *pPdu, uint16_t * usLen);
#endif

#if MB_FUNC_WRITE_HOLDING_ENABLED > 0
eMBException eMBFuncWriteHoldingRegister(mb_reg_t *regs, uint8_t *pPdu, uint16_t * usLen);
#endif

#if MB_FUNC_WRITE_MULTIPLE_HOLDING_ENABLED > 0
eMBException eMBFuncWriteMultipleHoldingRegister(mb_reg_t *regs, uint8_t *pPdu, uint16_t *usLen);
#endif

#if MB_FUNC_READ_COILS_ENABLED > 0
eMBException eMBFuncReadCoils(mb_reg_t *regs, uint8_t *pPdu, uint16_t *usLen );
#endif

#if MB_FUNC_WRITE_COIL_ENABLED > 0
eMBException eMBFuncWriteCoil(mb_reg_t *regs, uint8_t *pPdu, uint16_t *usLen);
#endif

#if MB_FUNC_WRITE_MULTIPLE_COILS_ENABLED > 0
eMBException eMBFuncWriteMultipleCoils(mb_reg_t *regs, uint8_t *pPdu, uint16_t *usLen);
#endif

#if MB_FUNC_READ_DISCRETE_INPUTS_ENABLED > 0
eMBException eMBFuncReadDiscreteInputs(mb_reg_t *regs, uint8_t *pPdu, uint16_t *usLen);
#endif

#if MB_FUNC_READWRITE_HOLDING_ENABLED > 0
eMBException eMBFuncReadWriteMultipleHoldingRegister(mb_reg_t *regs, uint8_t *pPdu, uint16_t *usLen);
#endif

#ifdef __cplusplus
}
#endif
#endif

