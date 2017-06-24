#include "mbfunc.h"

/*************************************************************************************************/
/* TODO implement modbus master */
#if MB_MASTER_ENABLED > 0
/* ok */
mb_ErrorCode_t eMBReqRdCoils(mb_MasterDevice_t *Mdev, uint8_t slaveaddr, 
                                uint16_t RegStartAddr, uint16_t Coilcnt, uint16_t scanrate)
{
    uint8_t *pAdu;
    uint16_t len;
    mb_request_t *req;
    mb_slavenode_t *node = NULL;
    mb_ErrorCode_t status;
    
    /* check slave address valid */
    if(slaveaddr > MB_ADDRESS_MAX) 
        return MBM_EINNODEADDR;
    /* check request count range( 1 - 2000 ) */
    if(Coilcnt < MB_READBITS_CNT_MIN || Coilcnt > MB_READBITS_CNT_MAX )
        return MB_EINVAL;
    /* if slave address not a broadcast address, search in the host?*/
    if(slaveaddr != MB_ADDRESS_BROADCAST){
        /* check node in host list */
        node = xMBMasterNodeSearch(Mdev,slaveaddr);
        if(node == NULL)
            return MBM_ENODENOSETUP;
        
        /* check register addres in range*/
        if((RegStartAddr < node->regs.reg_coils_addr_start)
            || ((RegStartAddr + Coilcnt) > (node->regs.reg_coils_addr_start + node->regs.reg_coils_num)))
            return MB_ENOREG;
    }
    
    req = xMB_ReqBufNew(Mdev->currentMode, MB_PDU_SIZE_FUNCODE + MB_PDU_FUNC_READ_SIZE);
    if(req == NULL)
        return MBM_ENOMEM;

    pAdu = req->padu;
    // set header and get head size
    len = xMBsetHead(Mdev->currentMode,pAdu, slaveaddr, MB_PDU_SIZE_FUNCODE + MB_PDU_FUNC_READ_SIZE);

    pAdu[len + MB_PDU_FUNCODE_OFF]               = MB_FUNC_READ_COILS;
    pAdu[len + MB_PDU_FUNC_READ_ADDR_OFF]        = RegStartAddr >> 8;
    pAdu[len + MB_PDU_FUNC_READ_ADDR_OFF + 1]    = RegStartAddr;
    pAdu[len + MB_PDU_FUNC_READ_BITSCNT_OFF]     = Coilcnt >> 8;
    pAdu[len + MB_PDU_FUNC_READ_BITSCNT_OFF + 1] = Coilcnt;
    
    req->adulength = len + MB_PDU_SIZE_FUNCODE + MB_PDU_FUNC_READ_SIZE;

    req->node      = node;
    req->errcnt    = 0;
    req->slaveaddr = slaveaddr;
    req->funcode   = MB_FUNC_READ_COILS;
    req->regaddr   = RegStartAddr;
    req->regcnt    = Coilcnt;
    req->scanrate  = ((scanrate < MBM_SCANRATE_MAX) ? scanrate : MBM_SCANRATE_MAX);
    req->scancnt   = 0;
    
    status = eMBMaster_Reqsend(Mdev,req);
    
    if(status != MB_ENOERR)
        vMB_ReqBufDelete(req);
    
    return status;
}
/* ok */
mb_ErrorCode_t eMBReqWrCoil(mb_MasterDevice_t *Mdev, uint8_t slaveaddr, 
                                uint16_t RegAddr, uint16_t val)
{
    uint8_t *pAdu;
    uint16_t len;
    mb_request_t *req;
    mb_slavenode_t *node = NULL;
    mb_ErrorCode_t status;

    /* check slave address valid */
    if(slaveaddr > MB_ADDRESS_MAX) 
        return MBM_EINNODEADDR;
    /* if slave address not a broadcast address, search in the host?*/
    if(slaveaddr != MB_ADDRESS_BROADCAST){
        /* check node in host list */
        node = xMBMasterNodeSearch(Mdev,slaveaddr);
        if(node == NULL)
            return MBM_ENODENOSETUP;
        /* check register addres in range*/
        if((RegAddr < node->regs.reg_coils_addr_start)
            || ((RegAddr + 1) > (node->regs.reg_coils_addr_start + node->regs.reg_coils_num)))
            return MB_ENOREG;
    }
    
    req = xMB_ReqBufNew(Mdev->currentMode, MB_PDU_SIZE_FUNCODE + MB_PDU_FUNC_WRITE_SIZE);
    if(req == NULL)
        return MBM_ENOMEM;
    
    pAdu = req->padu;
    // set header and get head size
    len = xMBsetHead(Mdev->currentMode,pAdu, slaveaddr, MB_PDU_SIZE_FUNCODE + MB_PDU_FUNC_WRITE_SIZE);

    val = (val > 0) ? 0xFF00 : 0x0000;
    pAdu[len + MB_PDU_FUNCODE_OFF]              = MB_FUNC_WRITE_SINGLE_COIL;
    pAdu[len + MB_PDU_FUNC_WRITE_ADDR_OFF]      = RegAddr >> 8;
    pAdu[len + MB_PDU_FUNC_WRITE_ADDR_OFF + 1]  = RegAddr;
    pAdu[len + MB_PDU_FUNC_WRITE_VALUE_OFF]     = val >> 8;
    pAdu[len + MB_PDU_FUNC_WRITE_VALUE_OFF + 1] = val;

    req->adulength = len + MB_PDU_SIZE_FUNCODE + MB_PDU_FUNC_WRITE_SIZE;

    req->node      = node;
    req->errcnt    = 0;
    req->slaveaddr = slaveaddr;
    req->funcode   = MB_FUNC_WRITE_SINGLE_COIL;
    req->regaddr   = RegAddr;
    req->regcnt    = 1;
    req->scanrate  = 0;
    req->scancnt   = 0;

    status = eMBMaster_Reqsend(Mdev,req);
    
    if(status != MB_ENOERR)
        vMB_ReqBufDelete(req);

    return status;
}
/* ok */
mb_ErrorCode_t eMbReqWrMulCoils(mb_MasterDevice_t *Mdev, uint8_t slaveaddr, 
                                        uint16_t RegStartAddr, uint16_t Coilcnt,
                                        uint8_t *valbuf, uint16_t valcnt)
{
    uint8_t *pAdu;
    uint16_t pdulengh,len;
    mb_request_t *req;
    mb_slavenode_t *node = NULL;
    mb_ErrorCode_t status;
    uint8_t ucByteCount;
    
    /* check slave address valid */
    if(slaveaddr > MB_ADDRESS_MAX) 
        return MBM_EINNODEADDR;
    /* check request count range( 1 - 2000 ) */    
    if( (Coilcnt < MB_WRITEBITS_CNT_MIN ) || (Coilcnt > MB_WRITEBITS_CNT_MAX ))
        return MB_EINVAL;
    
    /* Compute the number of expected bytes in the request. */
    ucByteCount = Coilcnt / 8 + (((Coilcnt & 0x0007) > 0) ? 1 : 0);

    if(ucByteCount != valcnt)
        return MB_EINVAL;
    /* if slave address not a broadcast address, search in the host?*/
    if(slaveaddr != MB_ADDRESS_BROADCAST){
        /* check node in host list */
        node = xMBMasterNodeSearch(Mdev,slaveaddr);
        if(node == NULL)
            return MBM_ENODENOSETUP;
        /* check register addres in range*/
        if((RegStartAddr < node->regs.reg_coils_addr_start)
            || ((RegStartAddr + Coilcnt) > (node->regs.reg_coils_addr_start + node->regs.reg_coils_num)))
            return MB_ENOREG;
    }
    
    /* slaveaddr +((PDU)funccode + startaddr + coilcnt + bytenum + value_list)  */
    pdulengh =  MB_PDU_SIZE_FUNCODE + MB_PDU_FUNC_WRITE_MUL_SIZE_MIN + ucByteCount;
    req = xMB_ReqBufNew(Mdev->currentMode, pdulengh);
    if(req == NULL)
        return MBM_ENOMEM;
    
    pAdu = req->padu;
    // get head size
    len = xMBsetHead(Mdev->currentMode,pAdu, slaveaddr, pdulengh);    

    pAdu[len + MB_PDU_FUNCODE_OFF]                      = MB_FUNC_WRITE_MULTIPLE_COILS;
    pAdu[len + MB_PDU_FUNC_WRITE_MUL_ADDR_OFF]          = RegStartAddr >> 8;
    pAdu[len + MB_PDU_FUNC_WRITE_MUL_ADDR_OFF + 1]      = RegStartAddr;
    pAdu[len + MB_PDU_FUNC_WRITE_MUL_COILCNT_OFF]       = Coilcnt >> 8;
    pAdu[len + MB_PDU_FUNC_WRITE_MUL_COILCNT_OFF + 1]   = Coilcnt;
    pAdu[len + MB_PDU_FUNC_WRITE_MUL_BYTECNT_OFF]       = ucByteCount;

    ucByteCount = 0;
    while(valcnt--)
    {
        pAdu[len + MB_PDU_FUNC_WRITE_MUL_VALUES_OFF + ucByteCount] = *valbuf;
        ucByteCount++;
        valbuf++;
    }
    req->adulength = len + pdulengh;

    req->node      = node;
    req->errcnt    = 0;
    req->slaveaddr = slaveaddr;
    req->funcode   = MB_FUNC_WRITE_MULTIPLE_COILS;
    req->regaddr   = RegStartAddr;
    req->regcnt    = Coilcnt;
    req->scanrate  = 0;
    req->scancnt   = 0;

    status = eMBMaster_Reqsend(Mdev,req);
    if(status != MB_ENOERR)
        vMB_ReqBufDelete(req);

    return status;
}

mb_ErrorCode_t eMBReqRdDiscreteInputs(mb_MasterDevice_t *Mdev, uint8_t slaveaddr, 
                                        uint16_t RegStartAddr, uint16_t Discnt, uint16_t scanrate)
{
    uint8_t *pAdu;
    uint16_t len;
    mb_request_t *req;
    mb_slavenode_t *node = NULL;
    mb_ErrorCode_t status;
    
    /* check slave address valid */
    if(slaveaddr > MB_ADDRESS_MAX) 
        return MBM_EINNODEADDR;
    /* check request count range( 1 - 2000 ) */
    if(Discnt < MB_READBITS_CNT_MIN || Discnt > MB_READBITS_CNT_MAX)
        return MB_EINVAL;
    /* if slave address not a broadcast address, search in the host?*/
    if(slaveaddr != MB_ADDRESS_BROADCAST){
        /* check node in host list */
        node = xMBMasterNodeSearch(Mdev,slaveaddr);
        if(node == NULL)
            return MBM_ENODENOSETUP;
        /* check register addres in range*/
        if((RegStartAddr < node->regs.reg_discrete_addr_start)
            || ((RegStartAddr + Discnt) > (node->regs.reg_discrete_addr_start + node->regs.reg_discrete_num)))
            return MB_ENOREG;
    }
    req = xMB_ReqBufNew(Mdev->currentMode, MB_PDU_SIZE_FUNCODE + MB_PDU_FUNC_READ_SIZE);
    if(pAdu == NULL)
        return MBM_ENOMEM;
    
    pAdu = req->padu;
    // set header and get head size
    len = xMBsetHead(Mdev->currentMode,pAdu, slaveaddr, MB_PDU_SIZE_FUNCODE + MB_PDU_FUNC_READ_SIZE);
    
    pAdu[len + MB_PDU_FUNCODE_OFF]               = MB_FUNC_READ_DISCRETE_INPUTS;
    pAdu[len + MB_PDU_FUNC_READ_ADDR_OFF]        = RegStartAddr >> 8;
    pAdu[len + MB_PDU_FUNC_READ_ADDR_OFF + 1]    = RegStartAddr;
    pAdu[len + MB_PDU_FUNC_READ_BITSCNT_OFF]     = Discnt >> 8;
    pAdu[len + MB_PDU_FUNC_READ_BITSCNT_OFF + 1] = Discnt;
    
    req->adulength = len + MB_PDU_SIZE_FUNCODE + MB_PDU_FUNC_READ_SIZE;

    req->node      = node;
    req->errcnt    = 0;
    req->slaveaddr = slaveaddr;
    req->funcode   = MB_FUNC_READ_DISCRETE_INPUTS;
    req->regaddr   = RegStartAddr;
    req->regcnt    = Discnt;
    req->scanrate  = ((scanrate < MBM_SCANRATE_MAX) ? scanrate : MBM_SCANRATE_MAX);
    req->scancnt   = 0;

    status = eMBMaster_Reqsend(Mdev,req);
    if(status != MB_ENOERR)
        vMB_ReqBufDelete(pAdu);

    return status;
}


/* TODO implement modbus master request parse */

/* write local bits register to coils or discrete */
static void __vMBLocalWrRegBits(uint8_t *pRegBits, uint16_t usStartAddress, uint8_t *pucRegBitsVal, uint16_t usNCoils)
{
    int16_t iNCoils = ( int16_t )usNCoils;

    while( iNCoils > 0 )
    {
        vMBSetBits(pRegBits, usStartAddress, (uint8_t)(iNCoils > 8 ? 8 : iNCoils), *pucRegBitsVal++);
        iNCoils -= 8;
        usStartAddress += 8;
    }
}
/* ok */
mb_ErrorCode_t eMBParseRspRdCoils(mb_Reg_t *regs, 
                                    uint16_t ReqRegAddr, uint16_t ReqRegcnt, 
                                    uint8_t *premain,uint16_t remainLength)
{
    uint8_t ucByteCount;

    ucByteCount = ReqRegcnt / 8 + (((ReqRegcnt & 0x0007) > 0) ? 1 : 0);
    /* check frame is right length */    
    /* check coilcnt with previous request byteNum */
    if((remainLength  != (1 + ucByteCount)) || (ucByteCount != premain[0]))
        return MB_EINVAL;
      
    __vMBLocalWrRegBits(regs->pRegCoil, ReqRegAddr - regs->reg_coils_addr_start,(uint8_t *)&premain[1],  ReqRegcnt);

    return MB_ENOERR;
}
/* ok */
mb_ErrorCode_t eMBParseRspWrCoil(mb_Reg_t *regs, 
                                    uint16_t ReqRegAddr, uint16_t ReqRegcnt,
                                    uint8_t *premain, uint16_t remainLength)
{
    uint8_t bitval = 0;

    (void) ReqRegcnt;
    if((remainLength != 4) || ReqRegAddr != ((premain[0] << 8) | premain[1]))
        return MB_EINVAL;

    
    if( ( premain[3] != 0x00 )
        || !((premain[2] == 0xFF) || (premain[2] == 0x00)) )
        return MB_EINVAL;
    
    if(premain[2] == 0xFF)
        bitval |= 0x01; 

    __vMBLocalWrRegBits(regs->pRegCoil, ReqRegAddr - regs->reg_coils_addr_start, (uint8_t *)&bitval, 1);

    return MB_ENOERR;
}
/* ok */
mb_ErrorCode_t eMBParseRspWrMulCoils(mb_Reg_t *regs, 
                                    uint16_t ReqRegAddr, uint16_t ReqRegcnt,
                                    uint8_t *premain, uint16_t remainLength)
{
    if(remainLength != 4)
        return MB_EINVAL;

    if((ReqRegAddr != ((premain[0] << 8) | premain[1]))
        || (ReqRegcnt != ((premain[2] << 8) | premain[3])))
        return MB_EINVAL;

    return MB_ENOERR;
}
/* ok */
mb_ErrorCode_t eMBParseRspRdDiscreteInputs(mb_Reg_t *regs, 
                                    uint16_t ReqRegAddr, uint16_t ReqRegcnt, 
                                    uint8_t *premain, uint16_t remainLength)
{
    uint8_t ucByteCount;
    
    ucByteCount = ReqRegcnt / 8 + (((ReqRegcnt & 0x0007) > 0) ? 1 : 0);
    /* check frame is right length */
    /* check coilcnt with previous request byteNum */
    if((remainLength  != (1 + ucByteCount)) || (ucByteCount != premain[0]))
        return MB_EINVAL;
      
    __vMBLocalWrRegBits(regs->pRegDisc, ReqRegAddr - regs->reg_discrete_addr_start, (uint8_t *)&premain[1], ReqRegcnt);

    return MB_ENOERR;
}

#endif

