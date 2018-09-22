
#include "stdlib.h"
#include "string.h"

#include "mbfunc.h"

#if MBS_FUNC_OTHER_REP_SLAVEID_ENABLED > 0

static uint8_t    ucMBSlaveID[MBS_FUNC_OTHER_REP_SLAVEID_BUF];
static uint16_t   usMBSlaveIDLen;

mb_ErrorCode_t eMbsSetSlaveID(Mb_Reg_t *regs, uint8_t ucSlaveID, bool xIsRunning,
                uint8_t const *pucAdditional, uint16_t usAdditionalLen)
{
    mb_ErrorCode_t eStatus = MB_ENOERR;

    (void)regs;
    
    /* the first byte and second byte in the buffer is reserved for
     * the parameter ucSlaveID and the running flag. The rest of
     * the buffer is available for additional data. */
    if(usAdditionalLen + 2 < MBS_FUNC_OTHER_REP_SLAVEID_BUF){
        
        usMBSlaveIDLen = 0;
        ucMBSlaveID[usMBSlaveIDLen++] = ucSlaveID;
        ucMBSlaveID[usMBSlaveIDLen++] = (uint8_t)(xIsRunning ? 0xFF : 0x00);
        if(usAdditionalLen > 0){
            memcpy(&ucMBSlaveID[usMBSlaveIDLen], pucAdditional, (size_t)usAdditionalLen);
            usMBSlaveIDLen += usAdditionalLen;
        }
    }
    else{
        eStatus = MB_ENORES;
    }
    
    return eStatus;
}

eMBException_t eMbsFuncReportSlaveID(Mb_Reg_t *regs, uint8_t *pPdu, uint16_t *usLen)
{
    (void)regs;

    memcpy(&pPdu[MB_PDU_DATA_OFF], &ucMBSlaveID[0], (size_t)usMBSlaveIDLen);
    *usLen = (uint16_t)(MB_PDU_DATA_OFF + usMBSlaveIDLen);
    return MB_EX_NONE;
}

#endif
