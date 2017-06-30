
#include "mbfunc.h"

#if MB_MASTER_ENABLED > 0

typedef struct
{
    uint8_t ucFunctionCode;
    pxMBParseRspHandler pxHandler;
} xMBParseRspHandler;


static xMBParseRspHandler xParseRspHandlers[MBS_FUNC_HANDLERS_MAX] = {
#if MBM_PARSE_RSP_OTHER_REP_SLAVEID_ENABLED > 0
    {MB_FUNC_OTHER_REPORT_SLAVEID, NULL},
#endif
#if MBM_PARSE_RSP_READ_HOLDING_ENABLED > 0
    {MB_FUNC_READ_HOLDING_REGISTER, eMBMParseRspRdHoldingRegister},
#endif
#if MBM_PARSE_RSP_WRITE_HOLDING_ENABLED > 0
    {MB_FUNC_WRITE_REGISTER, eMBMParseRspWrHoldingRegister},
#endif
#if MBM_PARSE_RSP_WRITE_MULTIPLE_HOLDING_ENABLED > 0
    {MB_FUNC_WRITE_MULTIPLE_REGISTERS, eMBMParseRspWrMulHoldingRegister},
#endif
#if MBM_PARSE_RSP_READWRITE_HOLDING_ENABLED > 0
    {MB_FUNC_READWRITE_MULTIPLE_REGISTERS, eMBMParseRspRdWrMulHoldingRegister},
#endif
#if MBM_PARSE_RSP_READ_INPUT_ENABLED > 0
    {MB_FUNC_READ_INPUT_REGISTER, eMBMParseRdInputRegister},
#endif
#if MBM_PARSE_RSP_READ_COILS_ENABLED > 0
    {MB_FUNC_READ_COILS, eMBMParseRspRdCoils},
#endif
#if MBM_PARSE_RSP_WRITE_COIL_ENABLED > 0
    {MB_FUNC_WRITE_SINGLE_COIL, eMBMParseRspWrCoil},
#endif
#if MBM_PARSE_RSP_WRITE_MULTIPLE_COILS_ENABLED > 0
    {MB_FUNC_WRITE_MULTIPLE_COILS, eMBMParseRspWrMulCoils},
#endif
#if MBM_PARSE_RSP_READ_DISCRETE_INPUTS_ENABLED > 0
    {MB_FUNC_READ_DISCRETE_INPUTS, eMBMParseRspRdDiscreteInputs},
#endif
};
pxMBParseRspHandler xMBMasterSearchCB(uint8_t ucFunctionCode)
{
    int i;
    pxMBParseRspHandler srch = NULL;

    for( i = 0; i < MBM_PARSE_RSP_HANDLERS_MAX; i++ ){
        /* No more function handlers registered. Abort. */
        if( xParseRspHandlers[i].ucFunctionCode == 0 ){
            break;
        }
        else if(xParseRspHandlers[i].ucFunctionCode == ucFunctionCode){
            srch = xParseRspHandlers[i].pxHandler;
            break;
        }                
    }

    return srch;
}

#endif

