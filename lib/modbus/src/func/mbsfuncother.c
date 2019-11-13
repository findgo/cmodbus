
#include "stdlib.h"
#include "string.h"

#include "mbfunc.h"

// TODO 功能未知,暂未实现
static uint8_t MbSlaveID[MBS_FUNC_OTHER_REP_SLAVEID_BUF];
static uint16_t MbSlaveIDLen;

MbErrCode_t MbsSetSlaveID(MbReg_t *pRegs, uint8_t slaveID, uint8_t isRunning,
                          uint8_t const *pAdditional, uint16_t additionalLen) {
    (void) pRegs;
    MbErrCode_t status = MB_ENOERR;

    /* the first byte and second byte in the buffer is reserved for
     * the parameter slaveID and the running flag. The rest of
     * the buffer is available for additional data. */
    if (additionalLen + 2 < MBS_FUNC_OTHER_REP_SLAVEID_BUF) {
        MbSlaveIDLen = 0;
        MbSlaveID[MbSlaveIDLen++] = slaveID;
        MbSlaveID[MbSlaveIDLen++] = (uint8_t) (isRunning ? 0xFF : 0x00);
        if (additionalLen > 0) {
            memcpy(&MbSlaveID[MbSlaveIDLen], pAdditional, (size_t) additionalLen);
            MbSlaveIDLen += additionalLen;
        }
    } else {
        status = MB_ENORES;
    }

    return status;
}

MbException_t MbsFuncReportSlaveID(MbReg_t *pRegs, uint8_t *pPdu, uint16_t *len) {
    (void) pRegs;

    memcpy(&pPdu[MB_PDU_DATA_OFF], &MbSlaveID[0], (size_t) MbSlaveIDLen);
    *len = (uint16_t) (MB_PDU_DATA_OFF + MbSlaveIDLen);

    return MB_EX_NONE;
}

