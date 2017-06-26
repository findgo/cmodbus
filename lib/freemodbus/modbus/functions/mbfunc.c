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
static xMBFunctionHandler xFuncHandlers[MB_FUNC_HANDLERS_MAX] = {
#if MB_FUNC_OTHER_REP_SLAVEID_ENABLED > 0
    {MB_FUNC_OTHER_REPORT_SLAVEID, eMBFuncReportSlaveID},
#endif
#if MB_FUNC_READ_HOLDING_ENABLED > 0
    {MB_FUNC_READ_HOLDING_REGISTER, eMBFuncRdHoldingRegister},
#endif
#if MB_FUNC_WRITE_HOLDING_ENABLED > 0
    {MB_FUNC_WRITE_REGISTER, eMBFuncWrHoldingRegister},
#endif
#if MB_FUNC_WRITE_MULTIPLE_HOLDING_ENABLED > 0
    {MB_FUNC_WRITE_MULTIPLE_REGISTERS, eMBFuncWrMulHoldingRegister},
#endif
#if MB_FUNC_READWRITE_HOLDING_ENABLED > 0
    {MB_FUNC_READWRITE_MULTIPLE_REGISTERS, eMBFuncRdWrMulHoldingRegister},
#endif
#if MB_FUNC_READ_INPUT_ENABLED > 0
    {MB_FUNC_READ_INPUT_REGISTER, eMBFuncRdInputRegister},
#endif
#if MB_FUNC_READ_COILS_ENABLED > 0
    {MB_FUNC_READ_COILS, eMBFuncRdCoils},
#endif
#if MB_FUNC_WRITE_COIL_ENABLED > 0
    {MB_FUNC_WRITE_SINGLE_COIL, eMBFuncWrCoil},
#endif
#if MB_FUNC_WRITE_MULTIPLE_COILS_ENABLED > 0
    {MB_FUNC_WRITE_MULTIPLE_COILS, eMBFuncWrMulCoils},
#endif
#if MB_FUNC_READ_DISCRETE_INPUTS_ENABLED > 0
    {MB_FUNC_READ_DISCRETE_INPUTS, eMBFuncRdDiscreteInputs},
#endif
};

mb_ErrorCode_t eMBRegisterCB( uint8_t ucFunctionCode, pxMBFunctionHandler pxHandler )
{
    int i;
    mb_ErrorCode_t eStatus = MB_ENORES;

    if((ucFunctionCode < MB_FUNC_MIN) || (ucFunctionCode > MB_FUNC_MAX))
        return MB_EINVAL;

    ENTER_CRITICAL_SECTION();
    for( i = 0; i < MB_FUNC_HANDLERS_MAX; i++ ){
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
    EXIT_CRITICAL_SECTION();
    
    return eStatus;
}

pxMBFunctionHandler xMBSearchCB(uint8_t ucFunctionCode)
{
    int i;
    pxMBFunctionHandler srch = NULL;

    for( i = 0; i < MB_FUNC_HANDLERS_MAX; i++){
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

