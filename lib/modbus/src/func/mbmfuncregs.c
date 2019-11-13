
#include "mbfunc.h"
#include "modbus.h"

#if MB_MASTER_ENABLED > 0

#include "mbmbuf.h"

/* ok */
MbReqResult_t MbmReqRdHoldingRegister(MbmHandle_t dev, uint8_t slaveID,
                                      uint16_t RegStartAddr, uint16_t Regcnt, uint16_t scanrate) {
    uint8_t *pAdu;
    uint16_t len;
    MbmReq_t *req;
    MbmNode_t *node = NULL;
    MbReqResult_t result;

    /* check slave address valid */
    if (slaveID > MB_ADDRESS_MAX)
        return MBR_EINNODEADDR;
    /* check request count range( 0 - 0x7d ) */
    if (Regcnt < MB_READ_REGS_QUANTITY_MIN || Regcnt > MB_READ_REG_QUANTITY_MAX)
        return MBR_EINVAL;
    /* if slave address not a broadcast address, search in the host?*/
    if (slaveID != MB_ADDRESS_BROADCAST) {
        /* check node in host list */
        node = MbmSearchNode(dev, slaveID);
        if (node == NULL)
            return MBR_ENODENOSETUP;
        /* check register addres in range*/
        if ((RegStartAddr < node->pRegs.holdingAddrStart)
            || ((RegStartAddr + Regcnt) > (node->pRegs.holdingAddrStart + node->pRegs.holdingNum)))
            return MBR_ENOREG;
    }

    req = MbmReqMsgNew(((MbmDev_t *) dev)->mode, MB_PDU_SIZE_FUNCODE + MB_PDU_FUNC_READ_SIZE);
    if (req == NULL)
        return MBR_ENOMEM;

    pAdu = &(req->adu[0]);
    // set header and get head size
    len = MbmBuildHead(((MbmDev_t *) dev)->mode, 0, slaveID, pAdu, MB_PDU_SIZE_FUNCODE + MB_PDU_FUNC_READ_SIZE);

    pAdu[len + MB_PDU_FUNCODE_OFF] = MB_FUNC_READ_HOLDING_REGISTER;
    pAdu[len + MB_PDU_FUNC_READ_ADDR_OFF] = RegStartAddr >> 8;
    pAdu[len + MB_PDU_FUNC_READ_ADDR_OFF + 1] = RegStartAddr;
    pAdu[len + MB_PDU_FUNC_READ_REGCNT_OFF] = Regcnt >> 8;
    pAdu[len + MB_PDU_FUNC_READ_REGCNT_OFF + 1] = Regcnt;

    req->adulength = len + MB_PDU_SIZE_FUNCODE + MB_PDU_FUNC_READ_SIZE;

    req->node = node;
    req->errcnt = 0;
    req->slaveID = slaveID;
    req->funcode = MB_FUNC_READ_HOLDING_REGISTER;
    req->regaddr = RegStartAddr;
    req->regcnt = Regcnt;
    req->scanrate = ((scanrate < MBM_SCANRATE_MAX) ? scanrate : MBM_SCANRATE_MAX);
    req->scancnt = 0;

    result = MbmSend(dev, req);
    if (result != MBR_ENOERR)
        MbmReqMsgDelete(req);

    return result;
}

/* ok */
MbReqResult_t MbmReqWrHoldingRegister(MbmHandle_t dev, uint8_t slaveID,
                                      uint16_t RegAddr, uint16_t val) {
    uint8_t *pAdu;
    uint16_t len;
    MbmReq_t *req;
    MbmNode_t *node = NULL;
    MbReqResult_t result;

    /* check slave address valid */
    if (slaveID > MB_ADDRESS_MAX)
        return MBR_EINNODEADDR;
    /* if slave address not a broadcast address, search in the host?*/
    if (slaveID != MB_ADDRESS_BROADCAST) {
        /* check node in host list */
        node = MbmSearchNode(dev, slaveID);
        if (node == NULL)
            return MBR_ENODENOSETUP;
        /* check register addres in range*/
        if ((RegAddr < node->pRegs.holdingAddrStart)
            || ((RegAddr + 1) > (node->pRegs.holdingAddrStart + node->pRegs.holdingNum)))
            return MBR_ENOREG;
    }

    req = MbmReqMsgNew(((MbmDev_t *) dev)->mode, MB_PDU_SIZE_FUNCODE + MB_PDU_FUNC_WRITE_SIZE);
    if (req == NULL)
        return MBR_ENOMEM;

    pAdu = &(req->adu[0]);
    // set header and get head size
    len = MbmBuildHead(((MbmDev_t *) dev)->mode, 0, slaveID, pAdu, MB_PDU_SIZE_FUNCODE + MB_PDU_FUNC_WRITE_SIZE);

    pAdu[len + MB_PDU_FUNCODE_OFF] = MB_FUNC_WRITE_REGISTER;
    pAdu[len + MB_PDU_FUNC_WRITE_ADDR_OFF] = RegAddr >> 8;
    pAdu[len + MB_PDU_FUNC_WRITE_ADDR_OFF + 1] = RegAddr;
    pAdu[len + MB_PDU_FUNC_WRITE_VALUE_OFF] = val >> 8;
    pAdu[len + MB_PDU_FUNC_WRITE_VALUE_OFF + 1] = val;

    req->adulength = len + MB_PDU_SIZE_FUNCODE + MB_PDU_FUNC_WRITE_SIZE;

    req->node = node;
    req->errcnt = 0;
    req->slaveID = slaveID;
    req->funcode = MB_FUNC_WRITE_REGISTER;
    req->regaddr = RegAddr;
    req->regcnt = 1;
    req->scanrate = 0;
    req->scancnt = 0;

    result = MbmSend(dev, req);
    if (result != MBR_ENOERR)
        MbmReqMsgDelete(req);

    return result;
}

/* ok */
MbReqResult_t MbmReqWrMulHoldingRegister(MbmHandle_t dev, uint8_t slaveID,
                                         uint16_t RegStartAddr, uint16_t Regcnt,
                                         uint16_t *valbuf, uint16_t valcnt) {
    uint8_t *pAdu;
    uint16_t pdulengh, len;
    MbmReq_t *req;
    MbmNode_t *node = NULL;
    MbReqResult_t result;
    uint8_t ucByteCount;

    /* check slave address valid */
    if (slaveID > MB_ADDRESS_MAX)
        return MBR_EINNODEADDR;
    if ((valcnt < MB_WRITE_REGS_QUANTITY_MIN)
        || (valcnt > MB_WRITE_REGS_QUANTITY_MAX)
        || Regcnt != valcnt)
        return MBR_EINVAL;
    /* if slave address not a broadcast address, search in the host?*/
    if (slaveID != MB_ADDRESS_BROADCAST) {
        /* check node in host list */
        node = MbmSearchNode(dev, slaveID);
        if (node == NULL)
            return MBR_ENODENOSETUP;
        /* check register addres in range*/
        if ((RegStartAddr < node->pRegs.holdingAddrStart)
            || ((RegStartAddr + valcnt) > (node->pRegs.holdingAddrStart + node->pRegs.holdingNum)))
            return MBR_ENOREG;
    }
    /* slaveID +((PDU)funcode + startaddr + regcnt + bytenum + regvalue_list)  */
    pdulengh = MB_PDU_SIZE_FUNCODE + MB_PDU_FUNC_WRITE_MUL_SIZE_MIN + valcnt * 2;
    req = MbmReqMsgNew(((MbmDev_t *) dev)->mode, pdulengh);
    if (req == NULL)
        return MBR_ENOMEM;

    pAdu = &(req->adu[0]);
    // set header and get head size
    len = MbmBuildHead(((MbmDev_t *) dev)->mode, 0, slaveID, pAdu, pdulengh);

    pAdu[len + MB_PDU_FUNCODE_OFF] = MB_FUNC_WRITE_MULTIPLE_REGISTERS;
    pAdu[len + MB_PDU_FUNC_WRITE_MUL_ADDR_OFF] = RegStartAddr >> 8;
    pAdu[len + MB_PDU_FUNC_WRITE_MUL_ADDR_OFF + 1] = RegStartAddr;
    pAdu[len + MB_PDU_FUNC_WRITE_MUL_REGCNT_OFF] = Regcnt >> 8;
    pAdu[len + MB_PDU_FUNC_WRITE_MUL_REGCNT_OFF + 1] = Regcnt;
    pAdu[len + MB_PDU_FUNC_WRITE_MUL_BYTECNT_OFF] = Regcnt * 2;

    ucByteCount = 0;
    while (valcnt--) {
        pAdu[len + MB_PDU_FUNC_WRITE_MUL_VALUES_OFF + ucByteCount] = *valbuf >> 8;
        ucByteCount++;
        pAdu[len + MB_PDU_FUNC_WRITE_MUL_VALUES_OFF + ucByteCount] = *valbuf;
        ucByteCount++;
        valbuf++;
    }

    req->adulength = len + pdulengh;

    req->node = node;
    req->errcnt = 0;
    req->slaveID = slaveID;
    req->funcode = MB_FUNC_WRITE_MULTIPLE_REGISTERS;
    req->regaddr = RegStartAddr;
    req->regcnt = Regcnt;
    req->scanrate = 0;
    req->scancnt = 0;

    result = MbmSend(dev, req);
    if (result != MBR_ENOERR)
        MbmReqMsgDelete(req);

    return result;
}

/* ok */
MbReqResult_t MbmReqRdInputRegister(MbmHandle_t dev, uint8_t slaveID,
                                    uint16_t RegStartAddr, uint16_t Regcnt, uint16_t scanrate) {
    uint8_t *pAdu;
    uint16_t len;
    MbmReq_t *req;
    MbmNode_t *node = NULL;
    MbReqResult_t result;

    /* check slave address valid */
    if (slaveID > MB_ADDRESS_MAX)
        return MBR_EINNODEADDR;
    /* check request count range( 0 - 0x7d ) */
    if (Regcnt < MB_READ_REGS_QUANTITY_MIN || Regcnt > MB_READ_REG_QUANTITY_MAX)
        return MBR_EINVAL;
    /* if slave address not a broadcast address, search in the host?*/
    if (slaveID != MB_ADDRESS_BROADCAST) {
        /* check node in host list */
        node = MbmSearchNode(dev, slaveID);
        if (node == NULL)
            return MBR_ENODENOSETUP;
        /* check register addres in range*/
        if ((RegStartAddr < node->pRegs.inputAddrStart)
            || ((RegStartAddr + Regcnt) > (node->pRegs.inputAddrStart + node->pRegs.inputNum)))
            return MBR_ENOREG;
    }

    req = MbmReqMsgNew(((MbmDev_t *) dev)->mode, MB_PDU_SIZE_FUNCODE + MB_PDU_FUNC_READ_SIZE);
    if (req == NULL)
        return MBR_ENOMEM;

    pAdu = &(req->adu[0]);
    // set header and get head size
    len = MbmBuildHead(((MbmDev_t *) dev)->mode, 0, slaveID, pAdu, MB_PDU_SIZE_FUNCODE + MB_PDU_FUNC_READ_SIZE);

    pAdu[len + MB_PDU_FUNCODE_OFF] = MB_FUNC_READ_INPUT_REGISTER;
    pAdu[len + MB_PDU_FUNC_READ_ADDR_OFF] = RegStartAddr >> 8;
    pAdu[len + MB_PDU_FUNC_READ_ADDR_OFF + 1] = RegStartAddr;
    pAdu[len + MB_PDU_FUNC_READ_REGCNT_OFF] = Regcnt >> 8;
    pAdu[len + MB_PDU_FUNC_READ_REGCNT_OFF + 1] = Regcnt;

    req->adulength = len + MB_PDU_SIZE_FUNCODE + MB_PDU_FUNC_READ_SIZE;

    req->node = node;
    req->errcnt = 0;
    req->slaveID = slaveID;
    req->funcode = MB_FUNC_READ_INPUT_REGISTER;
    req->regaddr = RegStartAddr;
    req->regcnt = Regcnt;
    req->scanrate = ((scanrate < MBM_SCANRATE_MAX) ? scanrate : MBM_SCANRATE_MAX);
    req->scancnt = 0;

    result = MbmSend(dev, req);
    if (result != MBR_ENOERR)
        MbmReqMsgDelete(req);

    return result;
}

/* ok */
MbReqResult_t MbmReqRdWrMulHoldingRegister(MbmHandle_t dev, uint8_t slaveID,
                                           uint16_t RegReadStartAddr, uint16_t RegReadCnt,
                                           uint16_t RegWriteStartAddr, uint16_t RegWriteCnt,
                                           uint16_t *valbuf, uint16_t valcnt) {
    uint8_t *pAdu;
    uint16_t pdulengh, len;
    MbmReq_t *req;
    MbmNode_t *node = NULL;
    MbReqResult_t result;
    uint16_t ucbyteCount;

    /* check slave address valid */
    if (slaveID > MB_ADDRESS_MAX)
        return MBR_EINNODEADDR;

    if (RegReadCnt < MB_READWRITE_READ_REG_QUANTITY_MIN || RegReadCnt > MB_READWRITE_READ_REG_QUANTITY_MAX
        || (RegWriteCnt < MB_READWRITE_WRITE_REGS_QUANTITY_MIN) || (RegWriteCnt > MB_READWRITE_WRITEREG_CNT_MAX)
        || RegWriteCnt != valcnt)
        return MBR_EINVAL;
    /* if slave address not a broadcast address, search in the host?*/
    if (slaveID != MB_ADDRESS_BROADCAST) {
        /* check node in host list */
        node = MbmSearchNode(dev, slaveID);
        if (node == NULL)
            return MBR_ENODENOSETUP;

        /* check register addres in range*/
        if ((RegReadStartAddr < node->pRegs.holdingAddrStart)
            || ((RegReadStartAddr + RegReadCnt) > (node->pRegs.holdingAddrStart + node->pRegs.holdingNum)))
            return MBR_ENOREG;

        if ((RegWriteStartAddr < node->pRegs.holdingAddrStart)
            || ((RegWriteStartAddr + RegWriteCnt) > (node->pRegs.holdingAddrStart + node->pRegs.holdingNum)))
            return MBR_ENOREG;
    }

    /* slaveID +((PDU) funcode + Readstartaddr + Readregcnt
     *    + Writestartaddr + Writeregcnt + bytenum + Writeregvalue_list)  */
    pdulengh = MB_PDU_SIZE_FUNCODE + MB_PDU_FUNC_READWRITE_SIZE_MIN + valcnt * 2;
    req = MbmReqMsgNew(((MbmDev_t *) dev)->mode, pdulengh);
    if (req == NULL)
        return MBR_ENOMEM;

    pAdu = &(req->adu[0]);
    // set header and get head size
    len = MbmBuildHead(((MbmDev_t *) dev)->mode, 0, slaveID, pAdu, pdulengh);

    pAdu[len + MB_PDU_FUNCODE_OFF] = MB_FUNC_READWRITE_MULTIPLE_REGISTERS;
    pAdu[len + MB_PDU_FUNC_READWRITE_READ_ADDR_OFF] = RegReadStartAddr >> 8;
    pAdu[len + MB_PDU_FUNC_READWRITE_READ_ADDR_OFF + 1] = RegReadStartAddr;
    pAdu[len + MB_PDU_FUNC_READWRITE_READ_REGCNT_OFF] = RegReadCnt >> 8;
    pAdu[len + MB_PDU_FUNC_READWRITE_READ_REGCNT_OFF + 1] = RegReadCnt;
    pAdu[len + MB_PDU_FUNC_READWRITE_WRITE_ADDR_OFF] = RegWriteStartAddr >> 8;
    pAdu[len + MB_PDU_FUNC_READWRITE_WRITE_ADDR_OFF + 1] = RegWriteStartAddr;
    pAdu[len + MB_PDU_FUNC_READWRITE_WRITE_REGCNT_OFF] = RegWriteCnt >> 8;
    pAdu[len + MB_PDU_FUNC_READWRITE_WRITE_REGCNT_OFF + 1] = RegWriteCnt;
    pAdu[len + MB_PDU_FUNC_READWRITE_BYTECNT_OFF] = RegWriteCnt * 2;

    ucbyteCount = 0;
    while (valcnt--) {
        pAdu[len + MB_PDU_FUNC_READWRITE_WRITE_VALUES_OFF + ucbyteCount] = *valbuf >> 8;
        ucbyteCount++;
        pAdu[len + MB_PDU_FUNC_READWRITE_WRITE_VALUES_OFF + ucbyteCount] = *valbuf;
        ucbyteCount++;
        valbuf++;
    }

    req->adulength = len + pdulengh;

    req->node = node;
    req->errcnt = 0;
    req->slaveID = slaveID;
    req->funcode = MB_FUNC_READWRITE_MULTIPLE_REGISTERS;
    req->regaddr = RegReadStartAddr;
    req->regcnt = RegReadCnt;
    req->scanrate = 0;
    req->scancnt = 0;

    result = MbmSend(dev, req);
    if (result != MBR_ENOERR)
        MbmReqMsgDelete(req);

    return result;
}

/* ok */
void __MbmLocalWrRegRegs(uint16_t *pRegRegs, uint16_t usAddressidx, uint8_t *pucRegRegsVal, uint16_t usNRegs) {
    while (usNRegs > 0) {
        pRegRegs[usAddressidx] = *pucRegRegsVal++ << 8;
        pRegRegs[usAddressidx] |= *pucRegRegsVal++;
        usAddressidx++;
        usNRegs--;
    }
}

/* ok */
MbReqResult_t MbmParseRspRdHoldingRegister(MbReg_t *pRegs,
                                           uint16_t ReqRegAddr, uint16_t ReqRegcnt,
                                           uint8_t *premain, uint16_t remainLength) {

    /* check frame is right length */
    /* check ReqRegcnt with previous request byteNum */
    if ((remainLength != (1 + ReqRegcnt * 2)) || (premain[0] != ReqRegcnt * 2))
        return MBR_EINVAL;

    __MbmLocalWrRegRegs(pRegs->pHolding, ReqRegAddr - pRegs->holdingAddrStart, (uint8_t *) &premain[1], ReqRegcnt);

    return MBR_ENOERR;
}

/* ok */
MbReqResult_t MbmParseRspWrHoldingRegister(MbReg_t *pRegs,
                                           uint16_t ReqRegAddr, uint16_t ReqRegcnt,
                                           uint8_t *premain, uint16_t remainLength) {
    (void) ReqRegcnt;

    if (remainLength != 4)
        return MBR_EINVAL;

    if (ReqRegAddr != ((premain[0] << 8) | premain[1]))
        return MBR_EINVAL;

    __MbmLocalWrRegRegs(pRegs->pHolding, ReqRegAddr - pRegs->holdingAddrStart, (uint8_t *) &premain[2], 1);

    return MBR_ENOERR;
}

/* ok */
MbReqResult_t MbmParseRspWrMulHoldingRegister(MbReg_t *pRegs,
                                              uint16_t ReqRegAddr, uint16_t ReqRegcnt,
                                              uint8_t *premain, uint16_t remainLength) {
    if (remainLength != 4)
        return MBR_EINVAL;

    if ((ReqRegAddr != ((premain[0] << 8) | premain[1]))
        || (ReqRegcnt != ((premain[2] << 8) | premain[3])))
        return MBR_EINVAL;

    return MBR_ENOERR;
}

/* ok */
MbReqResult_t MbmParseRspRdWrMulHoldingRegister(MbReg_t *pRegs,
                                                uint16_t ReqRegAddr, uint16_t ReqRegcnt,
                                                uint8_t *premain, uint16_t remainLength) {

    return MbmParseRspRdHoldingRegister(pRegs, ReqRegAddr, ReqRegcnt, premain, remainLength);

}

/* ok */
MbReqResult_t MbmParseRdInputRegister(MbReg_t *pRegs,
                                      uint16_t ReqRegAddr, uint16_t ReqRegcnt,
                                      uint8_t *premain, uint16_t remainLength) {
    /* check frame is right length */
    /* check ReqRegcnt with previous request byteNum */
    if ((remainLength != (1 + ReqRegcnt * 2)) || (premain[0] != ReqRegcnt * 2))
        return MBR_EINVAL;

    __MbmLocalWrRegRegs(pRegs->pInput, ReqRegAddr - pRegs->inputAddrStart, (uint8_t *) &premain[1], ReqRegcnt);

    return MBR_ENOERR;
}

#endif

