

#include "mbfunc.h"


#if MB_SLAVE_ENABLED > 0

/**
  * @brief  线圈寄存器处理函数，线圈寄存器可读，可读可写
  * @param  pRegs          操作寄存器指针
  * @param  pRegBuffer  读操作---返回数据指针，写操作--返回数据指针
  * @param  address     寄存器起始地址
  * @param  usNRegs       寄存器长度
  * @param  mode         操作方式，读或者写
  * @return               错误状态
  */
static MbErrCode_t __MbsRegCoilsCB(MbReg_t *pRegs, uint8_t *pRegBuffer,
                                   uint16_t address, uint16_t quantity, MbRegMode_t mode) {
    int16_t nCoils = (int16_t) quantity;
    uint16_t bitOffset;

    if (((int16_t) address >= pRegs->coilsAddrStart) &&
        ((address + quantity) <= (pRegs->coilsAddrStart + pRegs->coilsNum))) {
        bitOffset = (uint16_t) (address - pRegs->coilsAddrStart);
        switch (mode) {
            case MB_REG_READ:
                while (nCoils > 0) {
                    *pRegBuffer++ = MbGetBits(pRegs->pCoil, bitOffset, (uint8_t) (nCoils > 8 ? 8 : nCoils));
                    nCoils -= 8;
                    bitOffset += 8;
                }
                break;

            case MB_REG_WRITE:
                while (nCoils > 0) {
                    MbSetBits(pRegs->pCoil, bitOffset, (uint8_t) (nCoils > 8 ? 8 : nCoils), *pRegBuffer++);
                    nCoils -= 8;
                    bitOffset += 8;
                }
                break;
        }

        return MB_ENOERR;
    }

    return MB_ENOREG;
}

/**
  * @brief  离散输入寄存器处理函数，只可读
  * @param  pRegs          操作寄存器指针
  * @param  pRegBuffer  返回数据指针
  * @param  address     寄存器起始地址
  * @param  quantity       寄存器长度
  * @return               错误状态
  */
static MbErrCode_t __MbsRegDiscreteCB(MbReg_t *pRegs, uint8_t *pRegBuffer, uint16_t address, uint16_t quantity) {
    int16_t nDiscrete = (int16_t) quantity;
    uint16_t bitOffset;

    if (((int16_t) address >= pRegs->discreteAddrStart)
        && (address + quantity <= pRegs->discreteAddrStart + pRegs->discreteNum)) {
        bitOffset = (uint16_t) (address - pRegs->discreteAddrStart);
        while (nDiscrete > 0) {
            *pRegBuffer++ = MbGetBits(pRegs->pDiscrete, bitOffset, (uint8_t) (nDiscrete > 8 ? 8 : nDiscrete));
            nDiscrete -= 8;
            bitOffset += 8;
        }

        return MB_ENOERR;
    }

    return MB_ENOREG;
}

/**
* @brief   function handlers:  read coils register 
* @param   pRegs - real slave register pointer
* @param   pPdu - pdu frame pointer 
* @param   pLen - usLen pdu frame length pointer
* @return  exception code , see mbproto.h
*/
MbException_t MbsFuncRdCoils(MbReg_t *pRegs, uint8_t *pPdu, uint16_t *pLen) {
    uint16_t address;
    uint16_t quantity;
    uint8_t nBytes;
    uint8_t *pFrameCur;
    MbException_t status = MB_EX_NONE;
    MbErrCode_t regStatus;

    if (*pLen == (MB_PDU_FUNC_READ_SIZE + MB_PDU_SIZE_MIN)) {
        address = (uint16_t) (pPdu[MB_PDU_FUNC_READ_ADDR_OFF] << 8);
        address |= (uint16_t) (pPdu[MB_PDU_FUNC_READ_ADDR_OFF + 1]);
        quantity = (uint16_t) (pPdu[MB_PDU_FUNC_READ_BITSCNT_OFF] << 8);
        quantity |= (uint16_t) (pPdu[MB_PDU_FUNC_READ_BITSCNT_OFF + 1]);

        /* Check if the number of registers to read is valid. If not
         * return modbus illegal data value exception.
         */
        if ((quantity >= MB_READBITS_CNT_MIN) && (quantity < MB_READBITS_CNT_MAX)) {
            /* Set the current PDU data pointer to the beginning. */
            pFrameCur = &pPdu[MB_PDU_FUNCODE_OFF];
            /* First byte contains the function code. */
            *pFrameCur++ = MB_FUNC_READ_COILS;
            /* Test if the quantity of coils is a multiple of 8. If not last
             * byte is only partially field with unused coils set to zero. */
            nBytes = quantity / 8 + (((quantity & 0x0007) > 0) ? 1 : 0);
            *pFrameCur++ = nBytes;

            *pLen = MB_PDU_FUNCODE_OFF + 1 + 1;

            regStatus = __MbsRegCoilsCB(pRegs, pFrameCur, address, quantity, MB_REG_READ);
            /* If an error occured convert it into a Modbus exception. */
            if (regStatus != MB_ENOERR) {
                status = MbError2Exception(regStatus);
            } else {
                /* The response contains the function code, the starting address
                 * and the quantity of registers. We reuse the old values in the 
                 * buffer because they are still valid. */
                *pLen += nBytes;;
            }
        } else {
            status = MB_EX_ILLEGAL_DATA_VALUE;
        }
    } else {
        /* Can't be a valid read coil register request because the length
         * is incorrect. */
        status = MB_EX_ILLEGAL_DATA_VALUE;
    }

    return status;
}

/**
* @brief   function handlers:  write coils register 
* @param   pRegs - real slave register pointer
* @param   pPdu - pdu frame pointer 
* @param   pLen - usLen pdu frame length pointer
* @return  exception code , see mbproto.h
*/
MbException_t MbsFuncWrCoil(MbReg_t *pRegs, uint8_t *pPdu, uint16_t *pLen) {
    uint16_t address;
    uint8_t buf[2];
    MbException_t status = MB_EX_NONE;
    MbErrCode_t regStatus;

    if (*pLen == (MB_PDU_FUNC_WRITE_SIZE + MB_PDU_SIZE_MIN)) {
        address = (uint16_t) (pPdu[MB_PDU_FUNC_WRITE_ADDR_OFF] << 8);
        address |= (uint16_t) (pPdu[MB_PDU_FUNC_WRITE_ADDR_OFF + 1]);

        if ((pPdu[MB_PDU_FUNC_WRITE_VALUE_OFF + 1] == 0x00)
            && ((pPdu[MB_PDU_FUNC_WRITE_VALUE_OFF] == 0xFF) || (pPdu[MB_PDU_FUNC_WRITE_VALUE_OFF] == 0x00))) {
            buf[0] = (pPdu[MB_PDU_FUNC_WRITE_VALUE_OFF] == 0xFF) ? 1 : 0;
            buf[1] = 0;

            regStatus = __MbsRegCoilsCB(pRegs, &buf[0], address, 1, MB_REG_WRITE);
            /* If an error occurred convert it into a modbus exception. */
            if (regStatus != MB_ENOERR) {
                status = MbError2Exception(regStatus);
            }
        } else {
            status = MB_EX_ILLEGAL_DATA_VALUE;
        }
    } else {
        /* Can't be a valid write coil register request because the length is incorrect. */
        status = MB_EX_ILLEGAL_DATA_VALUE;
    }

    return status;
}

/**
* @brief   function handlers:  write multi coils register 
* @param   pRegs - real slave register pointer
* @param   pPdu - pdu frame pointer 
* @param   pLen - usLen pdu frame length pointer
* @return  exception code , see mbproto.h
*/
MbException_t MbsFuncWrMulCoils(MbReg_t *pRegs, uint8_t *pPdu, uint16_t *pLen) {
    uint16_t address;
    uint16_t quantity;
    uint8_t nBytes;
    uint8_t nBytesVerify;
    MbException_t status = MB_EX_NONE;
    MbErrCode_t regStatus;

    if (*pLen > (MB_PDU_FUNC_WRITE_SIZE + MB_PDU_SIZE_MIN)) {
        address = (uint16_t) (pPdu[MB_PDU_FUNC_WRITE_MUL_ADDR_OFF] << 8);
        address |= (uint16_t) (pPdu[MB_PDU_FUNC_WRITE_MUL_ADDR_OFF + 1]);
        quantity = (uint16_t) (pPdu[MB_PDU_FUNC_WRITE_MUL_COILCNT_OFF] << 8);
        quantity |= (uint16_t) (pPdu[MB_PDU_FUNC_WRITE_MUL_COILCNT_OFF + 1]);
        nBytes = pPdu[MB_PDU_FUNC_WRITE_MUL_BYTECNT_OFF];

        /* Compute the number of expected bytes in the request. */
        nBytesVerify = quantity / 8 + (((quantity & 0x0007) > 0) ? 1 : 0);

        if ((quantity >= MB_WRITEBITS_CNT_MIN) && (quantity <= MB_WRITEBITS_CNT_MAX) && (nBytesVerify == nBytes)) {
            regStatus = __MbsRegCoilsCB(pRegs, &pPdu[MB_PDU_FUNC_WRITE_MUL_VALUES_OFF],
                                        address, quantity, MB_REG_WRITE);
            /* If an error occured convert it into a Modbus exception. */
            if (regStatus != MB_ENOERR) {
                status = MbError2Exception(regStatus);
            } else {
                /* The response contains the function code, the starting address
                 * and the quantity of registers. We reuse the old values in the 
                 * buffer because they are still valid. */
                *pLen = MB_PDU_FUNC_WRITE_MUL_BYTECNT_OFF;
            }
        } else {
            status = MB_EX_ILLEGAL_DATA_VALUE;
        }
    } else {
        /* Can't be a valid write coil register request because the length
         * is incorrect. */
        status = MB_EX_ILLEGAL_DATA_VALUE;
    }

    return status;
}

/**
* @brief   function handlers:  read discrete imput register 
* @param   pRegs - real slave register pointer
* @param   pPdu - pdu frame pointer 
* @param   pLen - usLen pdu frame length pointer
* @return  exception code , see mbproto.h
*/
MbException_t MbsFuncRdDiscreteInputs(MbReg_t *pRegs, uint8_t *pPdu, uint16_t *pLen) {
    uint16_t address;
    uint16_t quantity;
    uint8_t nBytes;
    uint8_t *pFrameCur;
    MbException_t status = MB_EX_NONE;
    MbErrCode_t regStatus;

    if (*pLen == (MB_PDU_FUNC_READ_SIZE + MB_PDU_SIZE_MIN)) {
        address = (uint16_t) (pPdu[MB_PDU_FUNC_READ_ADDR_OFF] << 8);
        address |= (uint16_t) (pPdu[MB_PDU_FUNC_READ_ADDR_OFF + 1]);
        quantity = (uint16_t) (pPdu[MB_PDU_FUNC_READ_BITSCNT_OFF] << 8);
        quantity |= (uint16_t) (pPdu[MB_PDU_FUNC_READ_BITSCNT_OFF + 1]);

        /* Check if the number of registers to read is valid. If not
         * return Modbus illegal data value exception. 
         */
        if ((quantity >= MB_READBITS_CNT_MIN) && (quantity < MB_READBITS_CNT_MAX)) {
            /* Set the current PDU data pointer to the beginning. */
            pFrameCur = &pPdu[MB_PDU_FUNCODE_OFF];
            /* First byte contains the function code. */
            *pFrameCur++ = MB_FUNC_READ_DISCRETE_INPUTS;
            /* Test if the quantity of coils is a multiple of 8. If not last
             * byte is only partially field with unused coils set to zero. */
            nBytes = quantity / 8 + (((quantity & 0x0007) > 0) ? 1 : 0);
            *pFrameCur++ = nBytes;

            *pLen = MB_PDU_FUNCODE_OFF + 1 + 1;

            regStatus = __MbsRegDiscreteCB(pRegs, pFrameCur, address, quantity);
            /* If an error occurred convert it into a Modbus exception. */
            if (regStatus != MB_ENOERR) {
                status = MbError2Exception(regStatus);
            } else {
                /* The response contains the function code, the starting address
                 * and the quantity of registers. We reuse the old values in the 
                 * buffer because they are still valid. */
                *pLen += nBytes;;
            }
        } else {
            status = MB_EX_ILLEGAL_DATA_VALUE;
        }
    } else {
        /* Can't be a valid read coil register request because the length is incorrect. */
        status = MB_EX_ILLEGAL_DATA_VALUE;
    }

    return status;
}

#endif


