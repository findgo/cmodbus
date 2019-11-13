#include "mbfunc.h"

#if MB_SLAVE_ENABLED > 0

typedef struct {
    uint8_t functionCode;
    pMbsFunctionHandler pHandler;
} MbsFunctionHandler;

/* An array of modbus functions handlers which associates modbus function codes with implementing functions. */
static MbsFunctionHandler funcHandlers[MBS_FUNC_HANDLERS_MAX] = {
#if MBS_FUNC_OTHER_REP_SLAVEID_ENABLED > 0
        {MB_FUNC_OTHER_REPORT_SLAVEID, MbsFuncReportSlaveID},
#endif

#if MBS_FUNC_READ_HOLDING_ENABLED > 0
        {MB_FUNC_READ_HOLDING_REGISTER, MbsFuncRdHoldingRegister},
#endif

#if MBS_FUNC_WRITE_HOLDING_ENABLED > 0
        {MB_FUNC_WRITE_REGISTER, MbsFuncWrHoldingRegister},
#endif

#if MBS_FUNC_WRITE_MULTIPLE_HOLDING_ENABLED > 0
        {MB_FUNC_WRITE_MULTIPLE_REGISTERS, MbsFuncWrMulHoldingRegister},
#endif

#if MBS_FUNC_READWRITE_HOLDING_ENABLED > 0
        {MB_FUNC_READWRITE_MULTIPLE_REGISTERS, MbsFuncRdWrMulHoldingRegister},
#endif

#if MBS_FUNC_READ_INPUT_ENABLED > 0
        {MB_FUNC_READ_INPUT_REGISTER, MbsFuncRdInputRegister},
#endif

#if MBS_FUNC_READ_COILS_ENABLED > 0
        {MB_FUNC_READ_COILS, MbsFuncRdCoils},
#endif

#if MBS_FUNC_WRITE_COIL_ENABLED > 0
        {MB_FUNC_WRITE_SINGLE_COIL, MbsFuncWrCoil},
#endif

#if MBS_FUNC_WRITE_MULTIPLE_COILS_ENABLED > 0
        {MB_FUNC_WRITE_MULTIPLE_COILS, MbsFuncWrMulCoils},
#endif

#if MBS_FUNC_READ_DISCRETE_INPUTS_ENABLED > 0
        {MB_FUNC_READ_DISCRETE_INPUTS, MbsFuncRdDiscreteInputs},
#endif
};

/*********************************************************************
 * @brief   register function code handle
 * @param   functionCode - 功能码
 * @param   pHandler - 功能码对应的回调函数, NULL: 为注销对应功能码回调
 * @return  
 */

/**
 * @brief   register function code handle
 *
 * @param functionCode 功能码
 * @param pHandler 功能码对应的回调函数, NULL: 为注销对应功能码回调
 *
 * @return ::MbErrorCode_t
 */
MbErrCode_t MbsRegisterHandleCB(uint8_t functionCode, pMbsFunctionHandler pHandler) {
    int i;
    MbErrCode_t status = MB_ENORES;

    if ((functionCode < MB_FUNC_MIN) || (functionCode > MB_FUNC_MAX))
        return MB_EINVAL;

    for (i = 0; i < MBS_FUNC_HANDLERS_MAX; i++) {
        if ((funcHandlers[i].functionCode == 0) || (funcHandlers[i].functionCode == functionCode)) {
            // pHandler != NULL register,  NULL is unregister
            funcHandlers[i].functionCode = pHandler ? functionCode : 0;
            funcHandlers[i].pHandler = pHandler;

            status = MB_ENOERR;
            break;
        }
    }
    if (!pHandler) // remove can't failed!
        status = MB_ENOERR;

    return status;
}

/*
* @brief search function handle with function code
 *
* @param   functionCode - 功能码
* @param   pHandler - 功能码对应的回调函数, NULL: 为注销对应功能码回调
 *
* @return  function handle point, if not exist return NULL
*/
pMbsFunctionHandler MbsFuncHandleSearch(uint8_t functionCode) {
    int i;
    pMbsFunctionHandler search = NULL;

    for (i = 0; i < MBS_FUNC_HANDLERS_MAX; i++) {
        /* No more function handlers registered. Abort. */
        if (funcHandlers[i].functionCode == 0) {
            break;
        } else if (funcHandlers[i].functionCode == functionCode) {
            search = funcHandlers[i].pHandler;
        }
    }

    return search;
}

#endif

