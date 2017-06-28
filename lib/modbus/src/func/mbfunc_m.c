
#include "mbfunc.h"

#if MB_MASTER_ENABLED > 0

typedef struct
{
    uint8_t ucFunctionCode;
    pxMBParseRspHandler pxHandler;
} xMBParseRspHandler;


static xMBParseRspHandler xParseRspHandlers[MB_FUNC_HANDLERS_MAX] = {
#if MB_PARSE_RSP_OTHER_REP_SLAVEID_ENABLED > 0
    {MB_FUNC_OTHER_REPORT_SLAVEID, NULL},
#endif
#if MB_PARSE_RSP_READ_HOLDING_ENABLED > 0
    {MB_FUNC_READ_HOLDING_REGISTER, eMBParseRspRdHoldingRegister},
#endif
#if MB_PARSE_RSP_WRITE_HOLDING_ENABLED > 0
    {MB_FUNC_WRITE_REGISTER, eMBParseRspWrHoldingRegister},
#endif
#if MB_PARSE_RSP_WRITE_MULTIPLE_HOLDING_ENABLED > 0
    {MB_FUNC_WRITE_MULTIPLE_REGISTERS, eMBParseRspWrMulHoldingRegister},
#endif
#if MB_PARSE_RSP_READWRITE_HOLDING_ENABLED > 0
    {MB_FUNC_READWRITE_MULTIPLE_REGISTERS, eMBParseRspRdWrMulHoldingRegister},
#endif
#if MB_PARSE_RSP_READ_INPUT_ENABLED > 0
    {MB_FUNC_READ_INPUT_REGISTER, eMBParseRdInputRegister},
#endif
#if MB_PARSE_RSP_READ_COILS_ENABLED > 0
    {MB_FUNC_READ_COILS, eMBParseRspRdCoils},
#endif
#if MB_PARSE_RSP_WRITE_COIL_ENABLED > 0
    {MB_FUNC_WRITE_SINGLE_COIL, eMBParseRspWrCoil},
#endif
#if MB_PARSE_RSP_WRITE_MULTIPLE_COILS_ENABLED > 0
    {MB_FUNC_WRITE_MULTIPLE_COILS, eMBParseRspWrMulCoils},
#endif
#if MB_PARSE_RSP_READ_DISCRETE_INPUTS_ENABLED > 0
    {MB_FUNC_READ_DISCRETE_INPUTS, eMBParseRspRdDiscreteInputs},
#endif
};
pxMBParseRspHandler xMBMasterSearchCB(uint8_t ucFunctionCode)
{
    int i;
    pxMBParseRspHandler srch = NULL;

    for( i = 0; i < MB_PARSE_RSP_HANDLERS_MAX; i++ ){
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

