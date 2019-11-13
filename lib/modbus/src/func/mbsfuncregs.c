
#include "mbfunc.h"

#if MB_SLAVE_ENABLED > 0

/**
  * @brief  保持寄存器处理函数，保持寄存器可读，可读可写
  * @param  pRegs          操作寄存器指针
  * @param  pRegsBuffer  读操作时--返回数据指针，写操作时--输入数据指针
  * @param  address     寄存器起始地址
  * @param  regsNum       寄存器长度
  * @param  mode         操作方式，读或者写
  * @return              错误状态
  */
static MbErrorCode_t
__MbsRegHoldingCB(MbReg_t *pRegs, uint8_t *pRegsBuffer, uint16_t address, uint16_t regsNum, MbRegMode_t mode) {
    int16_t regIndex;

    if (((int16_t) address >= pRegs->holdingAddrStart)
        && ((address + regsNum) <= (pRegs->holdingAddrStart + pRegs->holdingNum))) {

        //offset index
        regIndex = (int16_t) (address - pRegs->holdingAddrStart);
        switch (mode) {
            case MB_REG_READ:
                while (regsNum > 0) {
                    //high byte
                    *pRegsBuffer++ = (uint8_t) (pRegs->pHolding[regIndex] >> 8);
                    //low byte
                    *pRegsBuffer++ = (uint8_t) (pRegs->pHolding[regIndex] & 0xFF);
                    regIndex++;
                    regsNum--;
                }
                break;

            case MB_REG_WRITE:
                while (regsNum > 0) {
                    pRegs->pHolding[regIndex] = *pRegsBuffer++ << 8;
                    pRegs->pHolding[regIndex] |= *pRegsBuffer++;
                    regIndex++;
                    regsNum--;
                }
                break;
        }

        return MB_ENOERR;
    }

    return MB_ENOREG;
}

/**
  * @brief  输入寄存器处理函数，输入寄存器可读，但不可写。
  * @param  pRegs          操作寄存器指针
  * @param  pRegBuffer  返回数据指针
  * @param  address     寄存器起始地址
  * @param  regsNum       寄存器长度
  * @return              错误状态
  */
static MbErrorCode_t __MbsRegInputCB(MbReg_t *pRegs, uint8_t *pRegBuffer, uint16_t address, uint16_t regsNum) {
    int16_t regIndex;

    if (((int16_t) address >= pRegs->inputAddrStart)
        && ((address + regsNum) <= (pRegs->inputAddrStart + pRegs->inputNum))) {

        //offset index
        regIndex = (int16_t) (address - pRegs->inputAddrStart);

        while (regsNum > 0) {
            //high byte
            *pRegBuffer++ = (uint8_t) (pRegs->pInput[regIndex] >> 8);
            //low byte
            *pRegBuffer++ = (uint8_t) (pRegs->pInput[regIndex] & 0xFF);
            regIndex++;
            regsNum--;
        }
        return MB_ENOERR;
    }

    return MB_ENOREG;
}

/**
 * @brief   function handlers:  read holding register 
 * @param   pRegs - real slave register pointer
 * @param   pPdu - pdu frame pointer 
 * @param   len - usLen pdu frame length pointer
 * @return  exception code , see mbproto.h
 */
MbException_t MbsFuncRdHoldingRegister(MbReg_t *pRegs, uint8_t *pPdu, uint16_t *len) {
    uint16_t regAddress;
    uint16_t regCount;
    uint8_t *pFrameCur;
    MbException_t status = MB_EX_NONE;
    MbErrorCode_t regStatus;

    if (*len == (MB_PDU_FUNC_READ_SIZE + MB_PDU_SIZE_MIN)) {
        regAddress = (uint16_t) (pPdu[MB_PDU_FUNC_READ_ADDR_OFF] << 8);
        regAddress |= (uint16_t) (pPdu[MB_PDU_FUNC_READ_ADDR_OFF + 1]);
        regCount = (uint16_t) (pPdu[MB_PDU_FUNC_READ_REGCNT_OFF] << 8);
        regCount |= (uint16_t) (pPdu[MB_PDU_FUNC_READ_REGCNT_OFF + 1]);

        /* Check if the number of registers to read is valid. If not
         * return Modbus illegal data value exception. 
         */
        if ((regCount >= MB_READREG_CNT_MIN) && (regCount <= MB_READREG_CNT_MAX)) {
            /* Set the current PDU data pointer to the beginning. */
            pFrameCur = &pPdu[MB_PDU_FUNCODE_OFF];
            *len = MB_PDU_FUNCODE_OFF;

            /* First byte contains the function code. */
            *pFrameCur++ = MB_FUNC_READ_HOLDING_REGISTER;
            *len += 1;

            /* Second byte in the response contain the number of bytes. */
            *pFrameCur++ = (uint8_t) (regCount * 2);
            *len += 1;

            /* Make callback to fill the buffer. */
            regStatus = __MbsRegHoldingCB(pRegs, pFrameCur, regAddress, regCount, MB_REG_READ);
            /* If an error occured convert it into a Modbus exception. */
            if (regStatus != MB_ENOERR) {
                status = MbError2Exception(regStatus);
            } else {
                *len += regCount * 2;
            }
        } else {
            status = MB_EX_ILLEGAL_DATA_VALUE;
        }
    } else {
        /* Can't be a valid request because the length is incorrect. */
        status = MB_EX_ILLEGAL_DATA_VALUE;
    }
    return status;
}

/**
 * @brief   function handlers:  write holding register 
 * @param   pRegs - real slave register pointer
 * @param   pPdu - pdu frame pointer 
 * @param   len - usLen pdu frame length pointer
 * @return  exception code , see mbproto.h
 */
MbException_t MbsFuncWrHoldingRegister(MbReg_t *pRegs, uint8_t *pPdu, uint16_t *len) {
    uint16_t regAddress;
    MbException_t status = MB_EX_NONE;
    MbErrorCode_t regStatus;

    if (*len == (MB_PDU_FUNC_WRITE_SIZE + MB_PDU_SIZE_MIN)) {
        regAddress = (uint16_t) (pPdu[MB_PDU_FUNC_WRITE_ADDR_OFF] << 8);
        regAddress |= (uint16_t) (pPdu[MB_PDU_FUNC_WRITE_ADDR_OFF + 1]);

        /* Make callback to update the value. */
        regStatus = __MbsRegHoldingCB(pRegs, &pPdu[MB_PDU_FUNC_WRITE_VALUE_OFF],
                                      regAddress, 1, MB_REG_WRITE);
        /* If an error occured convert it into a Modbus exception. */
        if (regStatus != MB_ENOERR) {
            status = MbError2Exception(regStatus);
        }
    } else {
        /* Can't be a valid request because the length is incorrect. */
        status = MB_EX_ILLEGAL_DATA_VALUE;
    }

    return status;
}

/**
 * @brief   function handlers:  write multi holding register 
 * @param   pRegs - real slave register pointer
 * @param   pPdu - pdu frame pointer 
 * @param   len - usLen pdu frame length pointer
 * @return  exception code , see mbproto.h
 */
MbException_t MbsFuncWrMulHoldingRegister(MbReg_t *pRegs, uint8_t *pPdu, uint16_t *len) {
    uint16_t regAddress;
    uint16_t regCount;
    uint8_t regByteCount;
    MbException_t status = MB_EX_NONE;
    MbErrorCode_t regStatus;

    if (*len >= (MB_PDU_FUNC_WRITE_MUL_SIZE_MIN + MB_PDU_SIZE_MIN)) {
        regAddress = (uint16_t) (pPdu[MB_PDU_FUNC_WRITE_MUL_ADDR_OFF] << 8);
        regAddress |= (uint16_t) (pPdu[MB_PDU_FUNC_WRITE_MUL_ADDR_OFF + 1]);

        regCount = (uint16_t) (pPdu[MB_PDU_FUNC_WRITE_MUL_REGCNT_OFF] << 8);
        regCount |= (uint16_t) (pPdu[MB_PDU_FUNC_WRITE_MUL_REGCNT_OFF + 1]);

        regByteCount = pPdu[MB_PDU_FUNC_WRITE_MUL_BYTECNT_OFF];
        if ((regCount >= MB_WRITEREG_CNT_MIN)
            && (regCount <= MB_WRITEREG_CNT_MAX)
            && (regByteCount == (uint8_t) (2 * regCount))) {

            /* Make callback to update the register values. */
            regStatus = __MbsRegHoldingCB(pRegs, &pPdu[MB_PDU_FUNC_WRITE_MUL_VALUES_OFF],
                                          regAddress, regCount, MB_REG_WRITE);
            /* If an error occured convert it into a Modbus exception. */
            if (regStatus != MB_ENOERR) {
                status = MbError2Exception(regStatus);
            } else {
                /* The response contains the function code, the starting
                 * address and the quantity of registers. We reuse the
                 * old values in the buffer because they are still valid.
                 */
                *len = MB_PDU_FUNC_WRITE_MUL_BYTECNT_OFF;
            }
        } else {
            status = MB_EX_ILLEGAL_DATA_VALUE;
        }
    } else {
        /* Can't be a valid request because the length is incorrect. */
        status = MB_EX_ILLEGAL_DATA_VALUE;
    }

    return status;
}

/**
 * @brief   function handlers:  reand and write multi holding register 
 * @param   pRegs - real slave register pointer
 * @param   pPdu - pdu frame pointer 
 * @param   len - usLen pdu frame length pointer
 * @return  exception code , see mbproto.h
 */
MbException_t MbsFuncRdWrMulHoldingRegister(MbReg_t *pRegs, uint8_t *pPdu, uint16_t *len) {
    uint16_t regReadAddress;
    uint16_t regReadCount;
    uint16_t regWriteAddress;
    uint16_t regWriteCount;
    uint8_t regWriteByteCount;
    uint8_t *pFrameCur;
    MbException_t status = MB_EX_NONE;
    MbErrorCode_t regStatus;

    if (*len >= (MB_PDU_FUNC_READWRITE_SIZE_MIN + MB_PDU_SIZE_MIN)) {
        regReadAddress = (uint16_t) (pPdu[MB_PDU_FUNC_READWRITE_READ_ADDR_OFF] << 8U);
        regReadAddress |= (uint16_t) (pPdu[MB_PDU_FUNC_READWRITE_READ_ADDR_OFF + 1]);

        regReadCount = (uint16_t) (pPdu[MB_PDU_FUNC_READWRITE_READ_REGCNT_OFF] << 8U);
        regReadCount |= (uint16_t) (pPdu[MB_PDU_FUNC_READWRITE_READ_REGCNT_OFF + 1]);

        regWriteAddress = (uint16_t) (pPdu[MB_PDU_FUNC_READWRITE_WRITE_ADDR_OFF] << 8U);
        regWriteAddress |= (uint16_t) (pPdu[MB_PDU_FUNC_READWRITE_WRITE_ADDR_OFF + 1]);

        regWriteCount = (uint16_t) (pPdu[MB_PDU_FUNC_READWRITE_WRITE_REGCNT_OFF] << 8U);
        regWriteCount |= (uint16_t) (pPdu[MB_PDU_FUNC_READWRITE_WRITE_REGCNT_OFF + 1]);

        regWriteByteCount = pPdu[MB_PDU_FUNC_READWRITE_BYTECNT_OFF];

        if ((regReadCount >= MB_READWRITE_READREG_CNT_MIN)
            && (regReadCount <= MB_READWRITE_READREG_CNT_MAX)
            && (regWriteCount >= MB_READWRITE_WRITEREG_CNT_MIN)
            && (regWriteCount <= MB_READWRITE_WRITEREG_CNT_MAX)
            && ((2 * regWriteCount) == regWriteByteCount)) {

            /* Make callback to update the register values. */
            regStatus = __MbsRegHoldingCB(pRegs, &pPdu[MB_PDU_FUNC_READWRITE_WRITE_VALUES_OFF],
                                          regWriteAddress, regWriteCount, MB_REG_WRITE);

            if (regStatus == MB_ENOERR) {
                /* Set the current PDU data pointer to the beginning. */
                pFrameCur = &pPdu[MB_PDU_FUNCODE_OFF];
                *len = MB_PDU_FUNCODE_OFF;

                /* First byte contains the function code. */
                *pFrameCur++ = MB_FUNC_READWRITE_MULTIPLE_REGISTERS;
                *len += 1;

                /* Second byte in the response contain the number of bytes. */
                *pFrameCur++ = (uint8_t) (regReadCount * 2);
                *len += 1;

                /* Make the read callback. */
                regStatus = __MbsRegHoldingCB(pRegs, pFrameCur, regReadAddress, regReadCount, MB_REG_READ);
                if (regStatus == MB_ENOERR) {
                    *len += 2 * regReadCount;
                }
            } else {
                status = MbError2Exception(regStatus);
            }
        } else {
            status = MB_EX_ILLEGAL_DATA_VALUE;
        }
    } else {
        /* Can't be a valid request because the length is incorrect. */
        status = MB_EX_ILLEGAL_DATA_VALUE;
    }

    return status;
}

/**
 * @brief   function handlers:  read input register 
 * @param   pRegs - real slave register pointer
 * @param   pPdu - pdu frame pointer 
 * @param   len - usLen pdu frame length pointer
 * @return  exception code , see mbproto.h
 */
MbException_t MbsFuncRdInputRegister(MbReg_t *pRegs, uint8_t *pPdu, uint16_t *len) {
    uint16_t regAddress;
    uint16_t regCount;
    uint8_t *pFrameCur;
    MbException_t status = MB_EX_NONE;
    MbErrorCode_t regStatus;

    if (*len == (MB_PDU_FUNC_READ_SIZE + MB_PDU_SIZE_MIN)) {
        regAddress = (uint16_t) (pPdu[MB_PDU_FUNC_READ_ADDR_OFF] << 8);
        regAddress |= (uint16_t) (pPdu[MB_PDU_FUNC_READ_ADDR_OFF + 1]);

        regCount = (uint16_t) (pPdu[MB_PDU_FUNC_READ_REGCNT_OFF] << 8);
        regCount |= (uint16_t) (pPdu[MB_PDU_FUNC_READ_REGCNT_OFF + 1]);

        /* Check if the number of registers to read is valid. If not
         * return Modbus illegal data value exception. 
         */
        if ((regCount >= MB_READREG_CNT_MIN) && (regCount < MB_READREG_CNT_MAX)) {
            /* Set the current PDU data pointer to the beginning. */
            pFrameCur = &pPdu[MB_PDU_FUNCODE_OFF];
            *len = MB_PDU_FUNCODE_OFF;

            /* First byte contains the function code. */
            *pFrameCur++ = MB_FUNC_READ_INPUT_REGISTER;
            *len += 1;

            /* Second byte in the response contain the number of bytes. */
            *pFrameCur++ = (uint8_t) (regCount * 2);
            *len += 1;

            regStatus = __MbsRegInputCB(pRegs, pFrameCur, regAddress, regCount);

            /* If an error occured convert it into a Modbus exception. */
            if (regStatus != MB_ENOERR) {
                status = MbError2Exception(regStatus);
            } else {
                *len += regCount * 2;
            }
        } else {
            status = MB_EX_ILLEGAL_DATA_VALUE;
        }
    } else {
        /* Can't be a valid read input register request because the length
         * is incorrect. */
        status = MB_EX_ILLEGAL_DATA_VALUE;
    }

    return status;
}

#endif


