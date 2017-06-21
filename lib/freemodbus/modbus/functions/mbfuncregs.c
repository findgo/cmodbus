
#include "mbfunc.h"

/**
  * @brief  保持寄存器处理函数，保持寄存器可读，可读可写
  * @param  regs          操作寄存器指针
  *         pucRegBuffer  读操作时--返回数据指针，写操作时--输入数据指针
  *         usAddress     寄存器起始地址
  *         usNRegs       寄存器长度
  *         eMode         操作方式，读或者写
  * @retval eStatus       寄存器状态
  */
static mb_ErrorCode_t __eMBRegHoldingCB(mb_Reg_t *regs, uint8_t *pucRegBuffer, uint16_t usAddress, uint16_t usNRegs, mb_RegisterMode_t eMode)
{
    int16_t iRegIndex;
  
    if( ((int16_t)usAddress >= regs->reg_holding_addr_start) \
        && ((usAddress + usNRegs) <= (regs->reg_holding_addr_start + regs->reg_holding_num))){
        
        //offset index
        iRegIndex = (int16_t)( usAddress - regs->reg_holding_addr_start);
        switch (eMode){
        case MB_REG_READ:
            while( usNRegs > 0 )
            {
                //high byte
                *pucRegBuffer++ = (uint8_t)( regs->pReghold[iRegIndex] >> 8 );
                //low byte
                *pucRegBuffer++ = (uint8_t)( regs->pReghold[iRegIndex] & 0xFF );
                iRegIndex++;
                usNRegs--;
            }
            break;

        case MB_REG_WRITE:
            while( usNRegs > 0 )
            {
                regs->pReghold[iRegIndex] = *pucRegBuffer++ << 8;
                regs->pReghold[iRegIndex] |= *pucRegBuffer++;
                iRegIndex++;
                usNRegs--;
            }
            break;
        }

        return MB_ENOERR;
    }
        
    return MB_ENOREG;
}
/**
  * @brief  输入寄存器处理函数，输入寄存器可读，但不可写。
  * @param  regs          操作寄存器指针
  *         pucRegBuffer  返回数据指针
  *         usAddress     寄存器起始地址
  *         usNRegs       寄存器长度
  * @retval eStatus       寄存器状态
  */

static mb_ErrorCode_t __eMBRegInputCB(mb_Reg_t *regs, uint8_t *pucRegBuffer, uint16_t usAddress, uint16_t usNRegs)
{
    int16_t iRegIndex;
  
    if(((int16_t) usAddress >= regs->reg_input_addr_start ) \
        && ( usAddress + usNRegs) <= regs->reg_input_addr_start + regs->reg_input_num ){
        
        //offset index
        iRegIndex = ( int16_t )( usAddress - regs->reg_input_addr_start);

        while( usNRegs > 0 )
        {
            //high byte
            *pucRegBuffer++ = ( uint8_t )( regs->pReginput[iRegIndex] >> 8 );
            //low byte
            *pucRegBuffer++ = ( uint8_t )( regs->pReginput[iRegIndex] & 0xFF );
            iRegIndex++;
            usNRegs--;
        }
        return MB_ENOERR;
    }

    return MB_ENOREG;
}

#if MB_FUNC_READ_HOLDING_ENABLED > 0
eMBException_t eMBFuncRdHoldingRegister(mb_Reg_t *regs, uint8_t * pPdu, uint16_t * usLen )
{
    uint16_t usRegAddress;
    uint16_t usRegCount;
    uint8_t *pucFrameCur;

    eMBException_t eStatus = MB_EX_NONE;
    mb_ErrorCode_t eRegStatus;

    if(*usLen == ( MB_PDU_FUNC_READ_SIZE + MB_PDU_SIZE_MIN )){
        
        usRegAddress = ( uint16_t )( pPdu[MB_PDU_FUNC_READ_ADDR_OFF] << 8 );
        usRegAddress |= ( uint16_t )( pPdu[MB_PDU_FUNC_READ_ADDR_OFF + 1] );

        usRegCount = ( uint16_t )( pPdu[MB_PDU_FUNC_READ_REGCNT_OFF] << 8 );
        usRegCount = ( uint16_t )( pPdu[MB_PDU_FUNC_READ_REGCNT_OFF + 1] );

        /* Check if the number of registers to read is valid. If not
         * return Modbus illegal data value exception. 
         */
        if((usRegCount >= MB_READREG_CNT_MIN) && (usRegCount <= MB_READREG_CNT_MAX)){
            
            /* Set the current PDU data pointer to the beginning. */
            pucFrameCur = &pPdu[MB_PDU_FUNCODE_OFF];
            *usLen = MB_PDU_FUNCODE_OFF;

            /* First byte contains the function code. */
            *pucFrameCur++ = MB_FUNC_READ_HOLDING_REGISTER;
            *usLen += 1;

            /* Second byte in the response contain the number of bytes. */
            *pucFrameCur++ = ( uint8_t ) ( usRegCount * 2 );
            *usLen += 1;

            /* Make callback to fill the buffer. */
            eRegStatus = __eMBRegHoldingCB(regs,pucFrameCur, usRegAddress, usRegCount, MB_REG_READ );
            /* If an error occured convert it into a Modbus exception. */
            if( eRegStatus != MB_ENOERR ){
                eStatus = prveMBError2Exception( eRegStatus );
            }
            else{
                *usLen += usRegCount * 2;
            }
        }
        else {
            eStatus = MB_EX_ILLEGAL_DATA_VALUE;
        }
    }
    else{
        /* Can't be a valid request because the length is incorrect. */
        eStatus = MB_EX_ILLEGAL_DATA_VALUE;
    }
    return eStatus;
}
#endif

#if MB_FUNC_WRITE_HOLDING_ENABLED > 0
eMBException_t eMBFuncWrHoldingRegister(mb_Reg_t *regs, uint8_t *pPdu, uint16_t *usLen )
{
    uint16_t usRegAddress;
    eMBException_t eStatus = MB_EX_NONE;
    mb_ErrorCode_t eRegStatus;

    if(*usLen == ( MB_PDU_FUNC_WRITE_SIZE + MB_PDU_SIZE_MIN )){
        
        usRegAddress = ( uint16_t )( pPdu[MB_PDU_FUNC_WRITE_ADDR_OFF] << 8 );
        usRegAddress |= ( uint16_t )( pPdu[MB_PDU_FUNC_WRITE_ADDR_OFF + 1] );

        /* Make callback to update the value. */
        eRegStatus = __eMBRegHoldingCB(regs,&pPdu[MB_PDU_FUNC_WRITE_VALUE_OFF],
                                      usRegAddress, 1, MB_REG_WRITE );

        /* If an error occured convert it into a Modbus exception. */
        if( eRegStatus != MB_ENOERR ){
            eStatus = prveMBError2Exception( eRegStatus );
        }
    }
    else{
        /* Can't be a valid request because the length is incorrect. */
        eStatus = MB_EX_ILLEGAL_DATA_VALUE;
    }
    
    return eStatus;
}
#endif

#if MB_FUNC_WRITE_MULTIPLE_HOLDING_ENABLED > 0
eMBException_t eMBFuncWrMulHoldingRegister(mb_Reg_t *regs, uint8_t * pPdu, uint16_t * usLen )
{
    uint16_t usRegAddress;
    uint16_t usRegCount;
    uint8_t  ucRegByteCount;

    eMBException_t eStatus = MB_EX_NONE;
    mb_ErrorCode_t eRegStatus;

    if( *usLen >= ( MB_PDU_FUNC_WRITE_MUL_SIZE_MIN + MB_PDU_SIZE_MIN ) ){
        
        usRegAddress = ( uint16_t )( pPdu[MB_PDU_FUNC_WRITE_MUL_ADDR_OFF] << 8 );
        usRegAddress |= ( uint16_t )( pPdu[MB_PDU_FUNC_WRITE_MUL_ADDR_OFF + 1] );

        usRegCount = ( uint16_t )( pPdu[MB_PDU_FUNC_WRITE_MUL_REGCNT_OFF] << 8 );
        usRegCount |= ( uint16_t )( pPdu[MB_PDU_FUNC_WRITE_MUL_REGCNT_OFF + 1] );

        ucRegByteCount = pPdu[MB_PDU_FUNC_WRITE_MUL_BYTECNT_OFF];
        if( ( usRegCount >= MB_WRITEREG_CNT_MIN )   \
            && ( usRegCount <= MB_WRITEREG_CNT_MAX )  \
            && ( ucRegByteCount == ( uint8_t ) ( 2 * usRegCount ))){
            
            /* Make callback to update the register values. */
            eRegStatus =
                __eMBRegHoldingCB(regs, &pPdu[MB_PDU_FUNC_WRITE_MUL_VALUES_OFF],
                                 usRegAddress, usRegCount, MB_REG_WRITE );

            /* If an error occured convert it into a Modbus exception. */
            if( eRegStatus != MB_ENOERR ){
                eStatus = prveMBError2Exception( eRegStatus );
            }
            else{
                /* The response contains the function code, the starting
                 * address and the quantity of registers. We reuse the
                 * old values in the buffer because they are still valid.
                 */
                *usLen = MB_PDU_FUNC_WRITE_MUL_BYTECNT_OFF;
            }
        }
        else{
            eStatus = MB_EX_ILLEGAL_DATA_VALUE;
        }
    }
    else{
        /* Can't be a valid request because the length is incorrect. */
        eStatus = MB_EX_ILLEGAL_DATA_VALUE;
    }
    
    return eStatus;
}
#endif

#if MB_FUNC_READWRITE_HOLDING_ENABLED > 0
eMBException_t eMBFuncRdWrMulHoldingRegister(mb_Reg_t *regs, uint8_t *pPdu, uint16_t *usLen )
{
    uint16_t usRegReadAddress;
    uint16_t usRegReadCount;
    uint16_t usRegWriteAddress;
    uint16_t usRegWriteCount;
    uint8_t  ucRegWriteByteCount;
    uint8_t *pucFrameCur;

    eMBException_t eStatus = MB_EX_NONE;
    mb_ErrorCode_t eRegStatus;

    if( *usLen >= ( MB_PDU_FUNC_READWRITE_SIZE_MIN + MB_PDU_SIZE_MIN ) ){
        
        usRegReadAddress = ( uint16_t )( pPdu[MB_PDU_FUNC_READWRITE_READ_ADDR_OFF] << 8U );
        usRegReadAddress |= ( uint16_t )( pPdu[MB_PDU_FUNC_READWRITE_READ_ADDR_OFF + 1] );

        usRegReadCount = ( uint16_t )( pPdu[MB_PDU_FUNC_READWRITE_READ_REGCNT_OFF] << 8U );
        usRegReadCount |= ( uint16_t )( pPdu[MB_PDU_FUNC_READWRITE_READ_REGCNT_OFF + 1] );

        usRegWriteAddress = ( uint16_t )( pPdu[MB_PDU_FUNC_READWRITE_WRITE_ADDR_OFF] << 8U );
        usRegWriteAddress |= ( uint16_t )( pPdu[MB_PDU_FUNC_READWRITE_WRITE_ADDR_OFF + 1] );

        usRegWriteCount = ( uint16_t )( pPdu[MB_PDU_FUNC_READWRITE_WRITE_REGCNT_OFF] << 8U );
        usRegWriteCount |= ( uint16_t )( pPdu[MB_PDU_FUNC_READWRITE_WRITE_REGCNT_OFF + 1] );

        ucRegWriteByteCount = pPdu[MB_PDU_FUNC_READWRITE_BYTECNT_OFF];

        if( ( usRegReadCount >= MB_READWRITE_READREG_CNT_MIN ) \
            && ( usRegReadCount <= MB_READWRITE_READREG_CNT_MAX ) \
            && ( usRegWriteCount >= MB_READWRITE_WRITEREG_CNT_MIN ) \
            && ( usRegWriteCount <= MB_READWRITE_WRITEREG_CNT_MAX ) \
            && ( ( 2 * usRegWriteCount ) == ucRegWriteByteCount ) ){
            
            /* Make callback to update the register values. */
            eRegStatus = __eMBRegHoldingCB(regs, &pPdu[MB_PDU_FUNC_READWRITE_WRITE_VALUES_OFF],
                                          usRegWriteAddress, usRegWriteCount, MB_REG_WRITE );

            if( eRegStatus == MB_ENOERR ){
                
                /* Set the current PDU data pointer to the beginning. */
                pucFrameCur = &pPdu[MB_PDU_FUNCODE_OFF];
                *usLen = MB_PDU_FUNCODE_OFF;

                /* First byte contains the function code. */
                *pucFrameCur++ = MB_FUNC_READWRITE_MULTIPLE_REGISTERS;
                *usLen += 1;

                /* Second byte in the response contain the number of bytes. */
                *pucFrameCur++ = ( uint8_t ) ( usRegReadCount * 2 );
                *usLen += 1;

                /* Make the read callback. */
                eRegStatus =
                    __eMBRegHoldingCB(regs, pucFrameCur, usRegReadAddress, usRegReadCount, MB_REG_READ );
                if( eRegStatus == MB_ENOERR ){
                    *usLen += 2 * usRegReadCount;
                }
            }
            if( eRegStatus != MB_ENOERR ){
                eStatus = prveMBError2Exception( eRegStatus );
            }
        }
        else{
            eStatus = MB_EX_ILLEGAL_DATA_VALUE;
        }
    }
    
    return eStatus;
}

#endif

#if MB_FUNC_READ_INPUT_ENABLED > 0
eMBException_t eMBFuncRdInputRegister(mb_Reg_t *regs, uint8_t * pPdu, uint16_t * usLen )
{
    uint16_t usRegAddress;
    uint16_t usRegCount;
    uint8_t *pucFrameCur;

    eMBException_t eStatus = MB_EX_NONE;
    mb_ErrorCode_t eRegStatus;

    if(*usLen == ( MB_PDU_FUNC_READ_SIZE + MB_PDU_SIZE_MIN )){
        
        usRegAddress = ( uint16_t )( pPdu[MB_PDU_FUNC_READ_ADDR_OFF] << 8 );
        usRegAddress |= ( uint16_t )( pPdu[MB_PDU_FUNC_READ_ADDR_OFF + 1] );

        usRegCount = ( uint16_t )( pPdu[MB_PDU_FUNC_READ_REGCNT_OFF] << 8 );
        usRegCount |= ( uint16_t )( pPdu[MB_PDU_FUNC_READ_REGCNT_OFF + 1] );

        /* Check if the number of registers to read is valid. If not
         * return Modbus illegal data value exception. 
         */
        if( ( usRegCount >= MB_READREG_CNT_MIN ) && ( usRegCount < MB_READREG_CNT_MAX ) ){
            
            /* Set the current PDU data pointer to the beginning. */
            pucFrameCur = &pPdu[MB_PDU_FUNCODE_OFF];
            *usLen = MB_PDU_FUNCODE_OFF;

            /* First byte contains the function code. */
            *pucFrameCur++ = MB_FUNC_READ_INPUT_REGISTER;
            *usLen += 1;

            /* Second byte in the response contain the number of bytes. */
            *pucFrameCur++ = ( uint8_t )( usRegCount * 2 );
            *usLen += 1;

            eRegStatus =
                __eMBRegInputCB(regs, pucFrameCur, usRegAddress, usRegCount );

            /* If an error occured convert it into a Modbus exception. */
            if( eRegStatus != MB_ENOERR ){
                eStatus = prveMBError2Exception( eRegStatus );
            }
            else{
                *usLen += usRegCount * 2;
            }
        }
        else{
            eStatus = MB_EX_ILLEGAL_DATA_VALUE;
        }
    }
    else{
        /* Can't be a valid read input register request because the length
         * is incorrect. */
        eStatus = MB_EX_ILLEGAL_DATA_VALUE;
    }
    
    return eStatus;
}

#endif

/*************************************************************************************************/
/* TODO implement modbus master */
#if MB_MASTER_ENABLE > 0

mb_ErrorCode_t eMBReqRdHoldingRegister(mb_MasterDevice_t *Mdev, uint8_t slaveaddr, 
                                        uint16_t RegStartAddr, uint16_t Regcnt, uint16_t scanrate)
{
    uint8_t *pAdu;
    uint16_t len;
    mb_request_t *req;
    mb_slavenode_t *node = NULL;
    mb_ErrorCode_t status;
    
    /* check slave address valid */
    if(slaveaddr > MB_ADDRESS_MAX) 
        return MBM_EINNODEADDR;
    /* check request count range( 0 - 0x7d ) */
    if(Regcnt > MB_READREG_CNT_MAX || Regcnt < MB_READREG_CNT_MIN)
        return MB_EINVAL;
    /* if slave address not a broadcast address, search in the host?*/
    if(slaveaddr != MB_ADDRESS_BROADCAST){
        /* check node in host list */
        node = xMBMasterNodeSearch(Mdev,slaveaddr);
        if(node == NULL)
            return MBM_ENODENOSETUP;
        /* check register addres in range*/
        if((RegStartAddr < node->regs.reg_holding_addr_start)
            || ((RegStartAddr + Regcnt) > (node->regs.reg_holding_addr_start + node->regs.reg_holding_num)))
            return MB_ENOREG;
    }
    
    req = xMB_ReqBufNew(Mdev->currentMode, MB_PDU_SIZE_FUNCODE + MB_PDU_FUNC_READ_SIZE);
    if(req == NULL)
        return MBM_ENOMEM;

    pAdu = req->padu;
    // set header and get head size
    len = xMBsetHead(Mdev->currentMode, pAdu, Mdev->currentMode, slaveaddr, MB_PDU_SIZE_FUNCODE + MB_PDU_FUNC_READ_SIZE);
    
    pAdu[len + MB_PDU_FUNCODE_OFF]              = MB_FUNC_READ_HOLDING_REGISTER;
    pAdu[len + MB_PDU_FUNC_READ_ADDR_OFF]       = RegStartAddr >> 8;
    pAdu[len + MB_PDU_FUNC_READ_ADDR_OFF + 1]   = RegStartAddr;
    pAdu[len + MB_PDU_FUNC_READ_REGCNT_OFF]     = Regcnt >> 8;
    pAdu[len + MB_PDU_FUNC_READ_REGCNT_OFF + 1] = Regcnt;
    
    req->adulength = len + MB_PDU_SIZE_FUNCODE + MB_PDU_FUNC_READ_SIZE;

    req->node      = node;
    req->errcnt    = 0;
    req->slaveaddr = slaveaddr;
    req->funcode   = MB_FUNC_READ_HOLDING_REGISTER;
    req->regaddr   = RegStartAddr;
    req->regcnt    = Regcnt;
    req->scanrate  = ((scanrate < MBM_SCANRATE_MAX) ? scanrate : MBM_SCANRATE_MAX);
    req->scancnt   = 0;

    status = eMBMaster_Reqsnd(Mdev, req);
    if(status != MB_ENOERR)
        vMB_ReqBufDelete(req);

    return status;
}

mb_ErrorCode_t eMBReqWrHoldingRegister(mb_MasterDevice_t *Mdev, uint8_t slaveaddr, 
                                        uint16_t RegAddr, uint16_t val, uint16_t scanrate)
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
        if((RegAddr < node->regs.reg_holding_addr_start)
            || ((RegAddr + 1) > (node->regs.reg_holding_addr_start + node->regs.reg_holding_num)))
            return MB_ENOREG;   
    }
    
    req = xMB_ReqBufNew(Mdev->currentMode, MB_PDU_SIZE_FUNCODE + MB_PDU_FUNC_WRITE_SIZE);
    if(pAdu == NULL)
        return MBM_ENOMEM;

    pAdu = req->padu;
    // set header and get head size
    len = xMBsetHead(Mdev->currentMode,pAdu, Mdev->currentMode, slaveaddr, MB_PDU_SIZE_FUNCODE + MB_PDU_FUNC_WRITE_SIZE);

    pAdu[len + MB_PDU_FUNCODE_OFF]              = MB_FUNC_WRITE_REGISTER;
    pAdu[len + MB_PDU_FUNC_WRITE_ADDR_OFF]      = RegAddr >> 8;
    pAdu[len + MB_PDU_FUNC_WRITE_ADDR_OFF + 1]  = RegAddr;
    pAdu[len + MB_PDU_FUNC_WRITE_VALUE_OFF]     = val >> 8;
    pAdu[len + MB_PDU_FUNC_WRITE_VALUE_OFF + 1] = val;

    req->adulength = len + MB_PDU_SIZE_FUNCODE + MB_PDU_FUNC_WRITE_SIZE;

    req->node      = node;
    req->errcnt    = 0;
    req->slaveaddr = slaveaddr;
    req->funcode   = MB_FUNC_WRITE_REGISTER;
    req->regaddr   = RegAddr;
    req->regcnt    = 1;
    req->scanrate  = ((scanrate < MBM_SCANRATE_MAX) ? scanrate : MBM_SCANRATE_MAX);
    req->scancnt   = 0;

    status = eMBMaster_Reqsnd(Mdev, req);
    if(status != MB_ENOERR)
        vMB_ReqBufDelete(req);

    return status;
}
mb_ErrorCode_t eMbReqWrMulHoldingRegister(mb_MasterDevice_t *Mdev, uint8_t slaveaddr, 
                                        uint16_t RegStartAddr, uint16_t Regcnt,
                                        uint16_t *valbuf, uint16_t valcnt, uint16_t scanrate)
{
    uint8_t *pAdu;
    uint16_t pdulengh,len;
    mb_request_t *req;
    mb_slavenode_t *node = NULL;
    mb_ErrorCode_t status;
    
    /* check slave address valid */
    if(slaveaddr > MB_ADDRESS_MAX) 
        return MBM_EINNODEADDR;
    if((valcnt < MB_WRITEREG_CNT_MIN ) 
        || (valcnt > MB_WRITEREG_CNT_MAX )
        || valcnt != valcnt)
        return MB_EINVAL;
    /* if slave address not a broadcast address, search in the host?*/
    if(slaveaddr != MB_ADDRESS_BROADCAST){
        /* check node in host list */
        node = xMBMasterNodeSearch(Mdev,slaveaddr);
        if(node == NULL)
            return MBM_ENODENOSETUP;
        /* check register addres in range*/
        if((RegStartAddr < node->regs.reg_holding_addr_start)
            || ((RegStartAddr + valcnt) > (node->regs.reg_holding_addr_start + node->regs.reg_holding_num)))
            return MB_ENOREG;   
    }    
    /* slaveaddr +((PDU)funcode + startaddr + regcnt + bytenum + regvalue_list)  */
    pdulengh = MB_PDU_SIZE_FUNCODE + MB_PDU_FUNC_WRITE_MUL_SIZE_MIN + valcnt * 2;
    req = xMB_ReqBufNew(Mdev->currentMode, pdulengh);
    if(req == NULL)
        return MBM_ENOMEM;

    pAdu = req->padu;
    // set header and get head size
    len = xMBsetHead(Mdev->currentMode,pAdu, Mdev->currentMode, slaveaddr, pdulengh);

    pAdu[len + MB_PDU_FUNCODE_OFF]                    = MB_FUNC_WRITE_MULTIPLE_REGISTERS;
    pAdu[len + MB_PDU_FUNC_WRITE_MUL_ADDR_OFF]        = RegStartAddr >> 8;
    pAdu[len + MB_PDU_FUNC_WRITE_MUL_ADDR_OFF + 1]    = RegStartAddr;
    pAdu[len + MB_PDU_FUNC_WRITE_MUL_REGCNT_OFF]      = Regcnt >> 8;
    pAdu[len + MB_PDU_FUNC_WRITE_MUL_REGCNT_OFF + 1]  = Regcnt;
    pAdu[len + MB_PDU_FUNC_WRITE_MUL_BYTECNT_OFF]     = valcnt * 2;

    valcnt = 0;
    while(Regcnt--)
    {
        pAdu[len + MB_PDU_FUNC_WRITE_MUL_VALUES_OFF + valcnt] = *valbuf++;
        ++valcnt;
        pAdu[len + MB_PDU_FUNC_WRITE_MUL_VALUES_OFF + valcnt] = *valbuf++;
        ++valcnt;
    }
    
    req->adulength = len + pdulengh;

    req->node      = node;
    req->errcnt    = 0;
    req->slaveaddr = slaveaddr;
    req->funcode   = MB_FUNC_WRITE_MULTIPLE_REGISTERS;
    req->regaddr   = RegStartAddr;
    req->regcnt    = Regcnt;
    req->scanrate  = ((scanrate < MBM_SCANRATE_MAX) ? scanrate : MBM_SCANRATE_MAX);
    req->scancnt   = 0;

    status = eMBMaster_Reqsnd(Mdev, req);
    if(status != MB_ENOERR)
        vMB_ReqBufDelete(req);
}

mb_ErrorCode_t eMBReqRdInputRegister(mb_MasterDevice_t *Mdev, uint8_t slaveaddr, 
                                        uint16_t RegStartAddr, uint16_t Regcnt, uint16_t scanrate)
{
    uint8_t *pAdu;
    uint16_t pdulengh,len;
    mb_request_t *req;
    mb_slavenode_t *node = NULL;
    mb_ErrorCode_t status;
    
    /* check slave address valid */
    if(slaveaddr > MB_ADDRESS_MAX) 
        return MBM_EINNODEADDR;
    /* check request count range( 0 - 0x7d ) */
    if(Regcnt > MB_READREG_CNT_MAX || Regcnt < MB_READREG_CNT_MIN)
        return MB_EINVAL;
    /* if slave address not a broadcast address, search in the host?*/
    if(slaveaddr != MB_ADDRESS_BROADCAST){
        /* check node in host list */
        node = xMBMasterNodeSearch(Mdev,slaveaddr);
        if(node == NULL)
            return MBM_ENODENOSETUP;
        /* check register addres in range*/
        if((RegStartAddr < node->regs.reg_input_addr_start)
            || ((RegStartAddr + Regcnt) > (node->regs.reg_input_addr_start + node->regs.reg_input_num)))
            return MB_ENOREG;
    }

    req = xMB_ReqBufNew(Mdev->currentMode, MB_PDU_SIZE_FUNCODE + MB_PDU_FUNC_READ_SIZE);
    if(req == NULL)
        return MBM_ENOMEM;

    pAdu = req->padu;
    // set header and get head size
    len = xMBsetHead(Mdev->currentMode,pAdu, Mdev->currentMode, slaveaddr, MB_PDU_SIZE_FUNCODE + MB_PDU_FUNC_READ_SIZE);

    pAdu[len + MB_PDU_FUNCODE_OFF]              = MB_FUNC_READ_INPUT_REGISTER;
    pAdu[len + MB_PDU_FUNC_READ_ADDR_OFF]       = RegStartAddr >> 8;
    pAdu[len + MB_PDU_FUNC_READ_ADDR_OFF + 1]   = RegStartAddr;
    pAdu[len + MB_PDU_FUNC_READ_REGCNT_OFF]     = Regcnt >> 8;
    pAdu[len + MB_PDU_FUNC_READ_REGCNT_OFF + 1] = Regcnt;

    req->adulength = len + MB_PDU_SIZE_FUNCODE + MB_PDU_FUNC_READ_SIZE;

    req->node      = node;
    req->errcnt    = 0;
    req->slaveaddr = slaveaddr;
    req->funcode   = MB_FUNC_READ_INPUT_REGISTER;
    req->regaddr   = RegStartAddr;
    req->regcnt    = Regcnt;
    req->scanrate  = ((scanrate < MBM_SCANRATE_MAX) ? scanrate : MBM_SCANRATE_MAX);
    req->scancnt   = 0;

    status = eMBMaster_Reqsnd(Mdev, req);
    if(status != MB_ENOERR)
        vMB_ReqBufDelete(req);

    return status;
}

eMBException_t eMBReqRdWrMulHoldingRegister(mb_MasterDevice_t *Mdev, uint8_t slaveaddr, 
                                                    uint16_t RegReadStartAddr, uint16_t RegReadCnt,
                                                    uint16_t RegWriteStartAddr, uint16_t RegWriteCnt,
                                                    uint16_t *valbuf, uint16_t valNUM,uint16_t scanrate)
{
    uint8_t *pAdu;
    uint16_t pdulengh,len;
    mb_request_t *req;
    mb_slavenode_t *node = NULL;
    mb_ErrorCode_t status;
    uint16_t byteCount;
    
    /* check slave address valid */
    if(slaveaddr > MB_ADDRESS_MAX) 
        return MBM_EINNODEADDR;
    
    if(RegReadCnt > MB_READWRITE_READREG_CNT_MIN || RegReadCnt < MB_READWRITE_READREG_CNT_MAX
        || ( RegWriteCnt < MB_READWRITE_WRITEREG_CNT_MIN ) || ( RegWriteCnt > MB_READWRITE_WRITEREG_CNT_MAX )
        ||  RegWriteCnt != valNUM)
        return MB_EINVAL;
    /* if slave address not a broadcast address, search in the host?*/
    if(slaveaddr != MB_ADDRESS_BROADCAST){
        /* check node in host list */
        node = xMBMasterNodeSearch(Mdev,slaveaddr);
        if(node == NULL)
            return MBM_ENODENOSETUP;

        /* check register addres in range*/
        if((RegReadStartAddr < node->regs.reg_holding_addr_start)
            || ((RegReadStartAddr + RegReadCnt) > (node->regs.reg_holding_addr_start + node->regs.reg_holding_num)))
            return MB_ENOREG;

        if((RegWriteStartAddr < node->regs.reg_holding_addr_start)
             || ((RegWriteStartAddr + RegWriteCnt) > (node->regs.reg_holding_addr_start + node->regs.reg_holding_num)))
            return MB_ENOREG;
    }
    
    /* slaveaddr +((PDU) funcode + Readstartaddr + Readregcnt 
     *    + Writestartaddr + Writeregcnt + bytenum + Writeregvalue_list)  */
    pdulengh = MB_PDU_SIZE_FUNCODE + MB_PDU_FUNC_READWRITE_SIZE_MIN + valNUM * 2;
    req = xMB_ReqBufNew(Mdev->currentMode, pdulengh);
    if(req == NULL)
        return MBM_ENOMEM;

    pAdu = req->padu;
    // set header and get head size
    len = xMBsetHead(Mdev->currentMode,pAdu, Mdev->currentMode, slaveaddr, pdulengh);

    pAdu[len + MB_PDU_FUNCODE_OFF]                         = MB_FUNC_READWRITE_MULTIPLE_REGISTERS;
    pAdu[len + MB_PDU_FUNC_READWRITE_READ_ADDR_OFF]        = RegReadStartAddr >> 8;
    pAdu[len + MB_PDU_FUNC_READWRITE_READ_ADDR_OFF + 1]    = RegReadStartAddr;
    pAdu[len + MB_PDU_FUNC_READWRITE_READ_REGCNT_OFF]      = RegReadCnt >> 8;
    pAdu[len + MB_PDU_FUNC_READWRITE_READ_REGCNT_OFF + 1]  = RegReadCnt;
    pAdu[len + MB_PDU_FUNC_READWRITE_WRITE_ADDR_OFF]       = RegWriteStartAddr >> 8;
    pAdu[len + MB_PDU_FUNC_READWRITE_WRITE_ADDR_OFF + 1]   = RegWriteStartAddr;
    pAdu[len + MB_PDU_FUNC_READWRITE_WRITE_REGCNT_OFF]     = RegWriteCnt >> 8;
    pAdu[len + MB_PDU_FUNC_READWRITE_WRITE_REGCNT_OFF + 1] = RegWriteCnt;
    pAdu[len + MB_PDU_FUNC_READWRITE_BYTECNT_OFF]          = RegWriteCnt * 2;

    byteCount = 0;
    while(RegWriteCnt--)
    {
        pAdu[len + MB_PDU_FUNC_READWRITE_WRITE_VALUES_OFF + byteCount] = *valbuf++;
        ++byteCount;
        pAdu[len + MB_PDU_FUNC_READWRITE_WRITE_VALUES_OFF + byteCount] = *valbuf++;
        ++byteCount;
    }

    req->adulength = len + pdulengh;

    req->node      = node;
    req->errcnt    = 0;
    req->slaveaddr = slaveaddr;
    req->funcode   = MB_FUNC_READWRITE_MULTIPLE_REGISTERS;
    req->regaddr   = RegReadStartAddr;
    req->regcnt    = RegReadCnt;
    req->scanrate  = ((scanrate < MBM_SCANRATE_MAX) ? scanrate : MBM_SCANRATE_MAX);
    req->scancnt   = 0;

    status = eMBMaster_Reqsnd(Mdev, req);
    if(status != MB_ENOERR)
        vMB_ReqBufDelete(req);

    return status;
}

void __vMBLocalWrRegRegs(uint16_t *pRegRegs, uint16_t usAddressidx, uint8_t *pucRegRegsVal, uint16_t usNRegs)
{
    while( usNRegs > 0 )
    {
        pRegRegs[usAddressidx] = *pucRegRegsVal++ << 8;
        pRegRegs[usAddressidx] |= *pucRegRegsVal++;
        usAddressidx++;
        usNRegs--;
    }
}

mb_ErrorCode_t eMBParseRspRdHoldingRegister(mb_Reg_t *regs, 
                                                uint16_t RegStartAddr, uint16_t Regcnt,
                                                uint8_t *premain, uint16_t remainLength)
{

    /* check frame is right length */
    /* check regcnt with previous request byteNum */
    if((remainLength  != (1 + Regcnt * 2)) || (premain[0] != Regcnt * 2))
        return MB_EINVAL; 
        
    __vMBLocalWrRegRegs(regs->pReghold, RegStartAddr - regs->reg_holding_addr_start, (uint8_t *)&premain[1], Regcnt);

    return MB_ENOERR;    
}
                                    
mb_ErrorCode_t eMBParseRspWrHoldingRegister(mb_Reg_t *regs, 
                                                    uint16_t RegAddr, uint16_t Regcnt,
                                                    uint8_t *premain, uint16_t remainLength)
{
    (void)Regcnt;
    
    if(remainLength != 4)
        return MB_EINVAL;

    if(RegAddr != ((premain[0] << 8) | premain[1]))
        return MB_EINVAL;

    __vMBLocalWrRegRegs(regs->pReghold, RegAddr - regs->reg_holding_addr_start, (uint8_t *)&premain[2], 1);
         
    return MB_ENOERR;   
}
                                                        
mb_ErrorCode_t eMBParseRspWrMulHoldingRegister(mb_Reg_t *regs, 
                                                        uint16_t RegStartAddr,uint16_t Regcnt, 
                                                        uint8_t *premain, uint16_t remainLength)
{
    if(remainLength != 4)
        return MB_EINVAL;

    if((RegStartAddr != ((premain[0] << 8) | premain[1]))
        || (Regcnt != ((premain[2] << 8) | premain[3])))
        return MB_EINVAL;

    return MB_ENOERR;    
}
mb_ErrorCode_t eMBParseRspRdWrMulHoldingRegister(mb_Reg_t *regs, 
                                                        uint16_t RegStartAddr,uint16_t Regcnt, 
                                                        uint8_t *premain, uint16_t remainLength)
{
    
    return eMBParseRspRdHoldingRegister(regs, RegStartAddr, Regcnt, premain, remainLength);
    
}
                                                        
mb_ErrorCode_t eMBParseRdInputRegister(mb_Reg_t *regs, 
                                            uint16_t RegStartAddr, uint16_t Regcnt,
                                            uint16_t *premain, uint16_t remainLength)
{
    /* check frame is right length */
    /* check regcnt with previous request byteNum */
    if((remainLength  != (1 + Regcnt * 2)) || (premain[0] != Regcnt * 2))
        return MB_EINVAL; 
        
    __vMBLocalWrRegRegs(regs->pReginput, RegStartAddr - regs->reg_input_addr_start, (uint8_t *)&premain[1], Regcnt);

    return MB_ENOERR;    
}

#endif

