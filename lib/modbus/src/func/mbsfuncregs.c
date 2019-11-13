
#include "mbfunc.h"

#if MB_SLAVE_ENABLED > 0

/**
  * @brief  保持寄存器处理函数，保持寄存器可读，可读可写
  * @param  pRegs          操作寄存器指针
  * @param  pRegsBuffer  读操作时--返回数据指针，写操作时--输入数据指针
  * @param  address     寄存器起始地址
  * @param  quantity       寄存器数量
  * @param  mode         操作方式，读或者写
  * @return              错误状态
  */
static MbErrorCode_t __MbsRegHoldingCB(MbReg_t *pRegs, uint8_t *pRegsBuffer,
                                       uint16_t address, uint16_t quantity, MbRegMode_t mode) {
    int16_t index;

    if (((int16_t) address >= pRegs->holdingAddrStart)
        && ((address + quantity) <= (pRegs->holdingAddrStart + pRegs->holdingNum))) {
        //offset index
        index = (int16_t) (address - pRegs->holdingAddrStart);
        switch (mode) {
            case MB_REG_READ:
                while (quantity > 0) {
                    //high byte
                    *pRegsBuffer++ = (uint8_t) (pRegs->pHolding[index] >> 8);
                    //low byte
                    *pRegsBuffer++ = (uint8_t) (pRegs->pHolding[index] & 0xFF);
                    index++;
                    quantity--;
                }
                break;

            case MB_REG_WRITE:
                while (quantity > 0) {
                    pRegs->pHolding[index] = *pRegsBuffer++ << 8;
                    pRegs->pHolding[index] |= *pRegsBuffer++;
                    index++;
                    quantity--;
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
  * @param  quantity       寄存器数量
  * @return              错误状态
  */
static MbErrorCode_t __MbsRegInputCB(MbReg_t *pRegs, uint8_t *pRegBuffer, uint16_t address, uint16_t quantity) {
    int16_t regIndex;

    if (((int16_t) address >= pRegs->inputAddrStart)
        && ((address + quantity) <= (pRegs->inputAddrStart + pRegs->inputNum))) {
        //offset index
        regIndex = (int16_t) (address - pRegs->inputAddrStart);

        while (quantity > 0) {
            //high byte
            *pRegBuffer++ = (uint8_t) (pRegs->pInput[regIndex] >> 8);
            //low byte
            *pRegBuffer++ = (uint8_t) (pRegs->pInput[regIndex] & 0xFF);
            regIndex++;
            quantity--;
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
    uint16_t address;
    uint16_t quantity;
    uint8_t *pFrameCur;
    MbException_t status = MB_EX_NONE;
    MbErrorCode_t regStatus;

    if (*len == (MB_PDU_FUNC_READ_SIZE + MB_PDU_SIZE_MIN)) {
        address = (uint16_t) (pPdu[MB_PDU_FUNC_READ_ADDR_OFF] << 8);
        address |= (uint16_t) (pPdu[MB_PDU_FUNC_READ_ADDR_OFF + 1]);
        quantity = (uint16_t) (pPdu[MB_PDU_FUNC_READ_REGCNT_OFF] << 8);
        quantity |= (uint16_t) (pPdu[MB_PDU_FUNC_READ_REGCNT_OFF + 1]);

        /* Check if the number of registers to read is valid. If not
         * return Modbus illegal data value exception. 
         */
        if ((quantity >= MB_READREG_CNT_MIN) && (quantity <= MB_READREG_CNT_MAX)) {
            /* Set the current PDU data pointer to the beginning. */
            pFrameCur = &pPdu[MB_PDU_FUNCODE_OFF];
            /* First byte contains the function code. */
            *pFrameCur++ = MB_FUNC_READ_HOLDING_REGISTER;
            /* Second byte in the response contain the number of bytes. */
            *pFrameCur++ = (uint8_t) (quantity * 2);

            *len = MB_PDU_FUNCODE_OFF + 1 + 1;

            /* Make callback to fill the buffer. */
            regStatus = __MbsRegHoldingCB(pRegs, pFrameCur, address, quantity, MB_REG_READ);
            /* If an error occured convert it into a Modbus exception. */
            if (regStatus != MB_ENOERR) {
                status = MbError2Exception(regStatus);
            } else {
                *len += quantity * 2;
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
    uint16_t address;
    MbException_t status = MB_EX_NONE;
    MbErrorCode_t regStatus;

    if (*len == (MB_PDU_FUNC_WRITE_SIZE + MB_PDU_SIZE_MIN)) {
        address = (uint16_t) (pPdu[MB_PDU_FUNC_WRITE_ADDR_OFF] << 8);
        address |= (uint16_t) (pPdu[MB_PDU_FUNC_WRITE_ADDR_OFF + 1]);

        /* Make callback to update the value. */
        regStatus = __MbsRegHoldingCB(pRegs, &pPdu[MB_PDU_FUNC_WRITE_VALUE_OFF],
                                      address, 1, MB_REG_WRITE);
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
    uint16_t address;
    uint16_t quantity;
    uint8_t nBytes;
    MbException_t status = MB_EX_NONE;
    MbErrorCode_t regStatus;

    if (*len >= (MB_PDU_FUNC_WRITE_MUL_SIZE_MIN + MB_PDU_SIZE_MIN)) {
        address = (uint16_t) (pPdu[MB_PDU_FUNC_WRITE_MUL_ADDR_OFF] << 8);
        address |= (uint16_t) (pPdu[MB_PDU_FUNC_WRITE_MUL_ADDR_OFF + 1]);
        quantity = (uint16_t) (pPdu[MB_PDU_FUNC_WRITE_MUL_REGCNT_OFF] << 8);
        quantity |= (uint16_t) (pPdu[MB_PDU_FUNC_WRITE_MUL_REGCNT_OFF + 1]);
        nBytes = pPdu[MB_PDU_FUNC_WRITE_MUL_BYTECNT_OFF];

        if ((quantity >= MB_WRITEREG_CNT_MIN) && (quantity <= MB_WRITEREG_CNT_MAX)
            && (nBytes == (uint8_t) (2 * quantity))) {
            /* Make callback to update the register values. */
            regStatus = __MbsRegHoldingCB(pRegs, &pPdu[MB_PDU_FUNC_WRITE_MUL_VALUES_OFF],
                                          address, quantity, MB_REG_WRITE);
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
    uint16_t readAddress;
    uint16_t readQuantity;
    uint16_t writeAddress;
    uint16_t writeQuantity;
    uint8_t WriteNBytes;
    uint8_t *pFrameCur;
    MbException_t status = MB_EX_NONE;
    MbErrorCode_t regStatus;

    if (*len >= (MB_PDU_FUNC_READWRITE_SIZE_MIN + MB_PDU_SIZE_MIN)) {
        readAddress = (uint16_t) (pPdu[MB_PDU_FUNC_READWRITE_READ_ADDR_OFF] << 8U);
        readAddress |= (uint16_t) (pPdu[MB_PDU_FUNC_READWRITE_READ_ADDR_OFF + 1]);
        readQuantity = (uint16_t) (pPdu[MB_PDU_FUNC_READWRITE_READ_REGCNT_OFF] << 8U);
        readQuantity |= (uint16_t) (pPdu[MB_PDU_FUNC_READWRITE_READ_REGCNT_OFF + 1]);
        writeAddress = (uint16_t) (pPdu[MB_PDU_FUNC_READWRITE_WRITE_ADDR_OFF] << 8U);
        writeAddress |= (uint16_t) (pPdu[MB_PDU_FUNC_READWRITE_WRITE_ADDR_OFF + 1]);
        writeQuantity = (uint16_t) (pPdu[MB_PDU_FUNC_READWRITE_WRITE_REGCNT_OFF] << 8U);
        writeQuantity |= (uint16_t) (pPdu[MB_PDU_FUNC_READWRITE_WRITE_REGCNT_OFF + 1]);
        WriteNBytes = pPdu[MB_PDU_FUNC_READWRITE_BYTECNT_OFF];

        if ((readQuantity >= MB_READWRITE_READREG_CNT_MIN) && (readQuantity <= MB_READWRITE_READREG_CNT_MAX)
            && (writeQuantity >= MB_READWRITE_WRITEREG_CNT_MIN) && (writeQuantity <= MB_READWRITE_WRITEREG_CNT_MAX)
            && ((2 * writeQuantity) == WriteNBytes)) {
            /* Make callback to update the register values. */
            regStatus = __MbsRegHoldingCB(pRegs, &pPdu[MB_PDU_FUNC_READWRITE_WRITE_VALUES_OFF],
                                          writeAddress, writeQuantity, MB_REG_WRITE);
            if (regStatus == MB_ENOERR) {
                /* Set the current PDU data pointer to the beginning. */
                pFrameCur = &pPdu[MB_PDU_FUNCODE_OFF];
                /* First byte contains the function code. */
                *pFrameCur++ = MB_FUNC_READWRITE_MULTIPLE_REGISTERS;
                /* Second byte in the response contain the number of bytes. */
                *pFrameCur++ = (uint8_t) (readQuantity * 2);

                *len = MB_PDU_FUNCODE_OFF + 1 + 1;

                /* Make the read callback. */
                regStatus = __MbsRegHoldingCB(pRegs, pFrameCur, readAddress, readQuantity, MB_REG_READ);
                /* If an error occured convert it into a Modbus exception. */
                if (regStatus != MB_ENOERR) {
                    status = MbError2Exception(regStatus);
                } else {
                    *len += 2 * readQuantity;
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
    uint16_t address;
    uint16_t quantity;
    uint8_t *pFrameCur;
    MbException_t status = MB_EX_NONE;
    MbErrorCode_t regStatus;

    if (*len == (MB_PDU_FUNC_READ_SIZE + MB_PDU_SIZE_MIN)) {
        address = (uint16_t) (pPdu[MB_PDU_FUNC_READ_ADDR_OFF] << 8);
        address |= (uint16_t) (pPdu[MB_PDU_FUNC_READ_ADDR_OFF + 1]);
        quantity = (uint16_t) (pPdu[MB_PDU_FUNC_READ_REGCNT_OFF] << 8);
        quantity |= (uint16_t) (pPdu[MB_PDU_FUNC_READ_REGCNT_OFF + 1]);

        /* Check if the number of registers to read is valid. If not return Modbus illegal data value exception. */
        if ((quantity >= MB_READREG_CNT_MIN) && (quantity < MB_READREG_CNT_MAX)) {

            /* Set the current PDU data pointer to the beginning. */
            pFrameCur = &pPdu[MB_PDU_FUNCODE_OFF];
            /* First byte contains the function code. */
            *pFrameCur++ = MB_FUNC_READ_INPUT_REGISTER;
            /* Second byte in the response contain the number of bytes. */
            *pFrameCur++ = (uint8_t) (quantity * 2);

            *len = MB_PDU_FUNCODE_OFF + 1 + 1;

            regStatus = __MbsRegInputCB(pRegs, pFrameCur, address, quantity);
            /* If an error occured convert it into a Modbus exception. */
            if (regStatus != MB_ENOERR) {
                status = MbError2Exception(regStatus);
            } else {
                *len += quantity * 2;
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


