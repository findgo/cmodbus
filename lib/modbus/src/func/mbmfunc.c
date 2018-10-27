
#include "mbfunc.h"

#if MB_MASTER_ENABLED > 0

typedef struct
{
    uint8_t ucFunctionCode;
    pMbmParseRspHandler pxHandler;
} MbmParseRspHandler;


static MbmParseRspHandler xParseRspHandlers[MBS_FUNC_HANDLERS_MAX] = {
#if MBM_PARSE_RSP_OTHER_REP_SLAVEID_ENABLED > 0
    {MB_FUNC_OTHER_REPORT_SLAVEID, NULL},
#endif
#if MBM_PARSE_RSP_READ_HOLDING_ENABLED > 0
    {MB_FUNC_READ_HOLDING_REGISTER, MbmParseRspRdHoldingRegister},
#endif
#if MBM_PARSE_RSP_WRITE_HOLDING_ENABLED > 0
    {MB_FUNC_WRITE_REGISTER, MbmParseRspWrHoldingRegister},
#endif
#if MBM_PARSE_RSP_WRITE_MULTIPLE_HOLDING_ENABLED > 0
    {MB_FUNC_WRITE_MULTIPLE_REGISTERS, MbmParseRspWrMulHoldingRegister},
#endif
#if MBM_PARSE_RSP_READWRITE_HOLDING_ENABLED > 0
    {MB_FUNC_READWRITE_MULTIPLE_REGISTERS, MbmParseRspRdWrMulHoldingRegister},
#endif
#if MBM_PARSE_RSP_READ_INPUT_ENABLED > 0
    {MB_FUNC_READ_INPUT_REGISTER, MbmParseRdInputRegister},
#endif
#if MBM_PARSE_RSP_READ_COILS_ENABLED > 0
    {MB_FUNC_READ_COILS, MbmParseRspRdCoils},
#endif
#if MBM_PARSE_RSP_WRITE_COIL_ENABLED > 0
    {MB_FUNC_WRITE_SINGLE_COIL, MbmParseRspWrCoil},
#endif
#if MBM_PARSE_RSP_WRITE_MULTIPLE_COILS_ENABLED > 0
    {MB_FUNC_WRITE_MULTIPLE_COILS, MbmParseRspWrMulCoils},
#endif
#if MBM_PARSE_RSP_READ_DISCRETE_INPUTS_ENABLED > 0
    {MB_FUNC_READ_DISCRETE_INPUTS, MbmParseRspRdDiscreteInputs},
#endif
};
// search function code handle
pMbmParseRspHandler MbmFuncHandleSearch(uint8_t ucFunctionCode)
{
    uint8_t i;

    for( i = 0; i < MBM_PARSE_RSP_HANDLERS_MAX; i++ ){
        /* No more function handlers registered. Abort. */
        if( xParseRspHandlers[i].ucFunctionCode == 0 ){
            return NULL;
        }
        else if(xParseRspHandlers[i].ucFunctionCode == ucFunctionCode){
            return (xParseRspHandlers[i].pxHandler);
        }                
    }

    return NULL;
}

#endif

