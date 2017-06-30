#include "mbfunc.h"

#if MB_SLAVE_ENABLED > 0

typedef struct
{
    uint8_t ucFunctionCode;
    pxMBFunctionHandler pxHandler;
} xMBFunctionHandler;

/* An array of Modbus functions handlers which associates Modbus function
 * codes with implementing functions.
 */
static xMBFunctionHandler xFuncHandlers[MBS_FUNC_HANDLERS_MAX] = {
#if MBS_FUNC_OTHER_REP_SLAVEID_ENABLED > 0
    {MB_FUNC_OTHER_REPORT_SLAVEID, eMbsFuncReportSlaveID},
#endif
#if MBS_FUNC_READ_HOLDING_ENABLED > 0
    {MB_FUNC_READ_HOLDING_REGISTER, eMbsFuncRdHoldingRegister},
#endif
#if MBS_FUNC_WRITE_HOLDING_ENABLED > 0
    {MB_FUNC_WRITE_REGISTER, eMbsFuncWrHoldingRegister},
#endif
#if MBS_FUNC_WRITE_MULTIPLE_HOLDING_ENABLED > 0
    {MB_FUNC_WRITE_MULTIPLE_REGISTERS, eMbsFuncWrMulHoldingRegister},
#endif
#if MBS_FUNC_READWRITE_HOLDING_ENABLED > 0
    {MB_FUNC_READWRITE_MULTIPLE_REGISTERS, eMbsFuncRdWrMulHoldingRegister},
#endif
#if MBS_FUNC_READ_INPUT_ENABLED > 0
    {MB_FUNC_READ_INPUT_REGISTER, eMbsFuncRdInputRegister},
#endif
#if MBS_FUNC_READ_COILS_ENABLED > 0
    {MB_FUNC_READ_COILS, eMbsFuncRdCoils},
#endif
#if MBS_FUNC_WRITE_COIL_ENABLED > 0
    {MB_FUNC_WRITE_SINGLE_COIL, eMbsFuncWrCoil},
#endif
#if MBS_FUNC_WRITE_MULTIPLE_COILS_ENABLED > 0
    {MB_FUNC_WRITE_MULTIPLE_COILS, eMbsFuncWrMulCoils},
#endif
#if MBS_FUNC_READ_DISCRETE_INPUTS_ENABLED > 0
    {MB_FUNC_READ_DISCRETE_INPUTS, eMbsFuncRdDiscreteInputs},
#endif
};

mb_ErrorCode_t eMbsRegisterCB( uint8_t ucFunctionCode, pxMBFunctionHandler pxHandler )
{
    int i;
    mb_ErrorCode_t eStatus = MB_ENORES;

    if((ucFunctionCode < MB_FUNC_MIN) || (ucFunctionCode > MB_FUNC_MAX))
        return MB_EINVAL;

    for( i = 0; i < MBS_FUNC_HANDLERS_MAX; i++ ){
        if((xFuncHandlers[i].ucFunctionCode == 0) 
            || (xFuncHandlers[i].ucFunctionCode == ucFunctionCode)){ 
            if( pxHandler){ // register
                xFuncHandlers[i].ucFunctionCode = ucFunctionCode;
                xFuncHandlers[i].pxHandler = pxHandler;            
            }
            else{   // unregister
                xFuncHandlers[i].ucFunctionCode = 0;
                xFuncHandlers[i].pxHandler = NULL;
            }
            eStatus = MB_ENOERR;
            break;
        }
    }
    if(!pxHandler) // remove can't failed!
        eStatus = MB_ENOERR;
    
    return eStatus;
}

pxMBFunctionHandler xMbsSearchCB(uint8_t ucFunctionCode)
{
    int i;
    pxMBFunctionHandler srch = NULL;

    for( i = 0; i < MBS_FUNC_HANDLERS_MAX; i++){
        /* No more function handlers registered. Abort. */
        if( xFuncHandlers[i].ucFunctionCode == 0 ){
            break;
        }
        else if(xFuncHandlers[i].ucFunctionCode == ucFunctionCode){
            srch = xFuncHandlers[i].pxHandler;
        }
    }

    return srch;  
}
#endif

