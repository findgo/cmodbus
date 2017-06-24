

#include "mbfunc.h"

#define BITS_uint8_t (8U)

/*! \defgroup modbus_utils Utilities
 *
 * This module contains some utility functions which can be used by
 * the application. It includes some special functions for working with
 * bitfields backed by a character array buffer.
 *
 */
/*! \addtogroup modbus_utils
 *  @{
 */
/*! \brief Function to set bits in a byte buffer.
 *
 * This function allows the efficient use of an array to implement bitfields.
 * The array used for storing the bits must always be a multiple of two
 * bytes. Up to eight bits can be set or cleared in one operation.
 *
 * \param ucByteBuf A buffer where the bit values are stored. Must be a
 *   multiple of 2 bytes. No length checking is performed and if
 *   usBitOffset / 8 is greater than the size of the buffer memory contents
 *   is overwritten.
 * \param usBitOffset The starting address of the bits to set. The first
 *   bit has the offset 0.
 * \param ucNBits Number of bits to modify. The value must always be smaller
 *   than 8.
 * \param ucValues Thew new values for the bits. The value for the first bit
 *   starting at <code>usBitOffset</code> is the LSB of the value
 *   <code>ucValues</code>
 *
 * \code
 * ucBits[2] = {0, 0};
 *
 * // Set bit 4 to 1 (read: set 1 bit starting at bit offset 4 to value 1)
 * vMBSetBits( ucBits, 4, 1, 1 );
 *
 * // Set bit 7 to 1 and bit 8 to 0.
 * vMBSetBits( ucBits, 7, 2, 0x01 );
 *
 * // Set bits 8 - 11 to 0x05 and bits 12 - 15 to 0x0A;
 * vMBSetBits( ucBits, 8, 8, 0x5A);
 * \endcode
 */
void vMBSetBits( uint8_t *ucByteBuf, uint16_t usBitOffset, uint8_t ucNBits, uint8_t ucValue )
{
    uint16_t usWordBuf;
    uint16_t usMask;
    uint16_t usByteOffset;
    uint16_t usNPreBits;
    uint16_t usValue = ucValue;

    assert( ucNBits <= 8 );
    assert( ( size_t )BITS_uint8_t == sizeof( uint8_t ) * 8 );

    /* Calculate byte offset for first byte containing the bit values starting
     * at usBitOffset. */
    usByteOffset = ( uint16_t )( ( usBitOffset ) / BITS_uint8_t );

    /* How many bits precede our bits to set. */
    usNPreBits = ( uint16_t )( usBitOffset - usByteOffset * BITS_uint8_t );

    /* Move bit field into position over bits to set */
    usValue <<= usNPreBits;

    /* Prepare a mask for setting the new bits. */
    usMask = ( uint16_t )( ( 1 << ( uint16_t ) ucNBits ) - 1 );
    usMask <<= usBitOffset - usByteOffset * BITS_uint8_t;

    /* copy bits into temporary storage. */
    usWordBuf = ucByteBuf[usByteOffset];
    usWordBuf |= ucByteBuf[usByteOffset + 1] << BITS_uint8_t;

    /* Zero out bit field bits and then or value bits into them. */
    usWordBuf = ( uint16_t )( ( usWordBuf & ( ~usMask ) ) | usValue );

    /* move bits back into storage */
    ucByteBuf[usByteOffset] = ( uint8_t )( usWordBuf & 0xFF );
    ucByteBuf[usByteOffset + 1] = ( uint8_t )( usWordBuf >> BITS_uint8_t );
}
                
/*! \brief Function to read bits in a byte buffer.
 *
 * This function is used to extract up bit values from an array. Up to eight
 * bit values can be extracted in one step.
 *
 * \param ucByteBuf A buffer where the bit values are stored.
 * \param usBitOffset The starting address of the bits to set. The first
 *   bit has the offset 0.
 * \param ucNBits Number of bits to modify. The value must always be smaller
 *   than 8.
 *
 * \code
 * uint8_t ucBits[2] = {0, 0};
 * uint8_t ucResult;
 *
 * // Extract the bits 3 - 10.
 * ucResult = xMBGetBits( ucBits, 3, 8 );
 * \endcode
 */
uint8_t xMBGetBits( uint8_t * ucByteBuf, uint16_t usBitOffset, uint8_t ucNBits )
{
    uint16_t usWordBuf;
    uint16_t usMask;
    uint16_t usByteOffset;
    uint16_t usNPreBits;

    /* Calculate byte offset for first byte containing the bit values starting
     * at usBitOffset. */
    usByteOffset = ( uint16_t )( ( usBitOffset ) / BITS_uint8_t );

    /* How many bits precede our bits to set. */
    usNPreBits = ( uint16_t )( usBitOffset - usByteOffset * BITS_uint8_t );

    /* Prepare a mask for setting the new bits. */
    usMask = ( uint16_t )( ( 1 << ( uint16_t ) ucNBits ) - 1 );

    /* copy bits into temporary storage. */
    usWordBuf = ucByteBuf[usByteOffset];
    usWordBuf |= ucByteBuf[usByteOffset + 1] << BITS_uint8_t;

    /* throw away unneeded bits. */
    usWordBuf >>= usNPreBits;

    /* mask away bits above the requested bitfield. */
    usWordBuf &= usMask;

    return ( uint8_t ) usWordBuf;
}

#if MB_SLAVE_ENABLE > 0
/**
  * @brief  线圈寄存器处理函数，线圈寄存器可读，可读可写
  * @param  regs          操作寄存器指针
  *         pucRegBuffer  读操作---返回数据指针，写操作--返回数据指针
  *         usAddress     寄存器起始地址
  *         usNRegs       寄存器长度
  *         eMode         操作方式，读或者写
  * @retval eStatus       寄存器状态
  */
  /* 根据定义，这里只处理线圈状态数据*/
static mb_ErrorCode_t __eMBRegCoilsCB(mb_Reg_t *regs, uint8_t *pucRegBuffer, uint16_t usAddress, uint16_t usNCoils, mb_RegisterMode_t eMode )
{
    int16_t iNCoils = ( int16_t )usNCoils;
    uint16_t usBitOffset;

    if( ((int16_t)usAddress >= regs->reg_coils_addr_start) && \
        ((usAddress + usNCoils) <= (regs->reg_coils_addr_start + regs->reg_coils_num))){

        usBitOffset = ( uint16_t )( usAddress - regs->reg_coils_addr_start );
        switch ( eMode ){
        case MB_REG_READ:
            while( iNCoils > 0 )
            {
                *pucRegBuffer++ = xMBGetBits( regs->pRegCoil, usBitOffset,( uint8_t )( iNCoils > 8 ? 8 : iNCoils ) );
                iNCoils -= 8;
                usBitOffset += 8;
            }
            break;

        case MB_REG_WRITE:
            while( iNCoils > 0 )
            {
                vMBSetBits(regs->pRegCoil, usBitOffset,( uint8_t )( iNCoils > 8 ? 8 : iNCoils ),*pucRegBuffer++);
                iNCoils -= 8;
                usBitOffset+=8;
            }
            break;
        }
        
        return MB_ENOERR;
    }
        
    return MB_ENOREG;
}

static mb_ErrorCode_t __eMBRegDiscreteCB(mb_Reg_t *regs, uint8_t * pucRegBuffer, uint16_t usAddress, uint16_t usNDiscrete )
{
  int16_t iNDiscrete = (int16_t)usNDiscrete;
  uint16_t usBitOffset;

    if(((int16_t)usAddress >= regs->reg_discrete_addr_start) \
        && (usAddress + usNDiscrete <= regs->reg_discrete_addr_start + regs->reg_discrete_num)){

        usBitOffset = ( uint16_t )(usAddress - regs->reg_discrete_addr_start);
        while( iNDiscrete > 0 )
        {
            *pucRegBuffer++ = xMBGetBits(regs->pRegDisc, usBitOffset,( uint8_t)(iNDiscrete > 8 ? 8 : iNDiscrete));
            iNDiscrete -= 8;
            usBitOffset += 8;
        }
        
        return MB_ENOERR;
    }

    return MB_ENOREG;
}

#if MB_FUNC_READ_COILS_ENABLED > 0
eMBException_t eMBFuncRdCoils(mb_Reg_t *regs, uint8_t *pPdu, uint16_t *usLen )
{
    uint16_t usRegAddress;
    uint16_t usCoilCount;
    uint8_t  ucNBytes;
    uint8_t  *pucFrameCur;

    eMBException_t eStatus = MB_EX_NONE;
    mb_ErrorCode_t eRegStatus;

    if( *usLen == ( MB_PDU_FUNC_READ_SIZE + MB_PDU_SIZE_MIN ) ){
        usRegAddress = ( uint16_t )( pPdu[MB_PDU_FUNC_READ_ADDR_OFF] << 8 );
        usRegAddress |= ( uint16_t )( pPdu[MB_PDU_FUNC_READ_ADDR_OFF + 1] );

        usCoilCount = ( uint16_t )( pPdu[MB_PDU_FUNC_READ_BITSCNT_OFF] << 8 );
        usCoilCount |= ( uint16_t )( pPdu[MB_PDU_FUNC_READ_BITSCNT_OFF + 1] );

        /* Check if the number of registers to read is valid. If not
         * return Modbus illegal data value exception. 
         */
        if( (usCoilCount >= MB_READBITS_CNT_MIN) && (usCoilCount < MB_READBITS_CNT_MAX)){
            
            /* Set the current PDU data pointer to the beginning. */
            pucFrameCur = &pPdu[MB_PDU_FUNCODE_OFF];
            *usLen = MB_PDU_FUNCODE_OFF;

            /* First byte contains the function code. */
            *pucFrameCur++ = MB_FUNC_READ_COILS;
            *usLen += 1;

            /* Test if the quantity of coils is a multiple of 8. If not last
             * byte is only partially field with unused coils set to zero. */
            ucNBytes = usCoilCount / 8 + (((usCoilCount & 0x0007) > 0) ? 1 : 0);

            *pucFrameCur++ = ucNBytes;
            *usLen += 1;

            eRegStatus =
                __eMBRegCoilsCB(regs, pucFrameCur, usRegAddress, usCoilCount,MB_REG_READ );

            /* If an error occured convert it into a Modbus exception. */
            if( eRegStatus != MB_ENOERR ){
                eStatus = prveMBError2Exception( eRegStatus );
            }
            else{
                /* The response contains the function code, the starting address
                 * and the quantity of registers. We reuse the old values in the 
                 * buffer because they are still valid. */
                *usLen += ucNBytes;;
            }
        }
        else{
            eStatus = MB_EX_ILLEGAL_DATA_VALUE;
        }
    }
    else{
        /* Can't be a valid read coil register request because the length
         * is incorrect. */
        eStatus = MB_EX_ILLEGAL_DATA_VALUE;
    }
    
    return eStatus;
}
#endif

#if MB_FUNC_WRITE_COIL_ENABLED > 0
eMBException_t eMBFuncWrCoil(mb_Reg_t *regs,uint8_t *pPdu, uint16_t * usLen)
{
    uint16_t usRegAddress;
    uint8_t  ucBuf[2];

    eMBException_t    eStatus = MB_EX_NONE;
    mb_ErrorCode_t    eRegStatus;

    if( *usLen == ( MB_PDU_FUNC_WRITE_SIZE + MB_PDU_SIZE_MIN ) ){
        
        usRegAddress = ( uint16_t )( pPdu[MB_PDU_FUNC_WRITE_ADDR_OFF] << 8 );
        usRegAddress |= ( uint16_t )( pPdu[MB_PDU_FUNC_WRITE_ADDR_OFF + 1] );

        if( ( pPdu[MB_PDU_FUNC_WRITE_VALUE_OFF + 1] == 0x00 ) \
            && ((pPdu[MB_PDU_FUNC_WRITE_VALUE_OFF] == 0xFF) \
            || (pPdu[MB_PDU_FUNC_WRITE_VALUE_OFF] == 0x00)) ){
            
            ucBuf[1] = 0;
            if( pPdu[MB_PDU_FUNC_WRITE_VALUE_OFF] == 0xFF ){
                ucBuf[0] = 1;
            }
            else{
                ucBuf[0] = 0;
            }
            eRegStatus =
                __eMBRegCoilsCB(regs,&ucBuf[0], usRegAddress, 1, MB_REG_WRITE );

            /* If an error occured convert it into a Modbus exception. */
            if( eRegStatus != MB_ENOERR ){
                eStatus = prveMBError2Exception( eRegStatus );
            }
        }
        else{
            eStatus = MB_EX_ILLEGAL_DATA_VALUE;
        }
    }
    else{
        /* Can't be a valid write coil register request because the length
         * is incorrect. */
        eStatus = MB_EX_ILLEGAL_DATA_VALUE;
    }
    
    return eStatus;
}
#endif

#if MB_FUNC_WRITE_MULTIPLE_COILS_ENABLED > 0
eMBException_t eMBFuncWrMulCoils(mb_Reg_t *regs,uint8_t * pPdu, uint16_t * usLen )
{
    uint16_t usRegAddress;
    uint16_t usCoilCnt;
    uint8_t  ucByteCount;
    uint8_t  ucByteCountVerify;

    eMBException_t    eStatus = MB_EX_NONE;
    mb_ErrorCode_t    eRegStatus;

    if(*usLen > (MB_PDU_FUNC_WRITE_SIZE + MB_PDU_SIZE_MIN )){
        
        usRegAddress = ( uint16_t )( pPdu[MB_PDU_FUNC_WRITE_MUL_ADDR_OFF] << 8 );
        usRegAddress |= ( uint16_t )( pPdu[MB_PDU_FUNC_WRITE_MUL_ADDR_OFF + 1] );

        usCoilCnt = ( uint16_t )( pPdu[MB_PDU_FUNC_WRITE_MUL_COILCNT_OFF] << 8 );
        usCoilCnt |= ( uint16_t )( pPdu[MB_PDU_FUNC_WRITE_MUL_COILCNT_OFF + 1] );

        ucByteCount = pPdu[MB_PDU_FUNC_WRITE_MUL_BYTECNT_OFF];

        /* Compute the number of expected bytes in the request. */
        ucByteCountVerify = usCoilCnt / 8 + (((usCoilCnt & 0x0007) > 0) ? 1 : 0);
        
        if( (usCoilCnt >= MB_WRITEBITS_CNT_MIN ) \
            && (usCoilCnt <= MB_WRITEBITS_CNT_MAX ) \
             && (ucByteCountVerify == ucByteCount ) ){
             
            eRegStatus =
                __eMBRegCoilsCB(regs,&pPdu[MB_PDU_FUNC_WRITE_MUL_VALUES_OFF], usRegAddress, usCoilCnt, MB_REG_WRITE );

            /* If an error occured convert it into a Modbus exception. */
            if( eRegStatus != MB_ENOERR ){
                eStatus = prveMBError2Exception( eRegStatus );
            }
            else{
                /* The response contains the function code, the starting address
                 * and the quantity of registers. We reuse the old values in the 
                 * buffer because they are still valid. */
                *usLen = MB_PDU_FUNC_WRITE_MUL_BYTECNT_OFF;
            }
        }
        else{
            eStatus = MB_EX_ILLEGAL_DATA_VALUE;
        }
    }
    else{
        /* Can't be a valid write coil register request because the length
         * is incorrect. */
        eStatus = MB_EX_ILLEGAL_DATA_VALUE;
    }
    
    return eStatus;
}
#endif

#if MB_FUNC_READ_DISCRETE_INPUTS_ENABLED > 0
eMBException_t eMBFuncRdDiscreteInputs(mb_Reg_t *regs, uint8_t * pPdu, uint16_t *usLen )
{
    uint16_t usRegAddress;
    uint16_t usDiscreteCnt;
    uint8_t  ucNBytes;
    uint8_t  *pucFrameCur;

    eMBException_t eStatus = MB_EX_NONE;
    mb_ErrorCode_t eRegStatus;

    if(*usLen == ( MB_PDU_FUNC_READ_SIZE + MB_PDU_SIZE_MIN )){

        usRegAddress = ( uint16_t )( pPdu[MB_PDU_FUNC_READ_ADDR_OFF] << 8 );
        usRegAddress |= ( uint16_t )( pPdu[MB_PDU_FUNC_READ_ADDR_OFF + 1] );

        usDiscreteCnt = ( uint16_t )( pPdu[MB_PDU_FUNC_READ_BITSCNT_OFF] << 8 );
        usDiscreteCnt |= ( uint16_t )( pPdu[MB_PDU_FUNC_READ_BITSCNT_OFF + 1] );

        /* Check if the number of registers to read is valid. If not
         * return Modbus illegal data value exception. 
         */
        if( (usDiscreteCnt >= MB_READBITS_CNT_MIN) && (usDiscreteCnt < MB_READBITS_CNT_MAX ) ){
            
            /* Set the current PDU data pointer to the beginning. */
            pucFrameCur = &pPdu[MB_PDU_FUNCODE_OFF];
            *usLen = MB_PDU_FUNCODE_OFF;

            /* First byte contains the function code. */
            *pucFrameCur++ = MB_FUNC_READ_DISCRETE_INPUTS;
            *usLen += 1;

            /* Test if the quantity of coils is a multiple of 8. If not last
             * byte is only partially field with unused coils set to zero. */
            ucNBytes = usDiscreteCnt / 8 + (((usDiscreteCnt & 0x0007) > 0) ? 1 : 0);
            
            *pucFrameCur++ = ucNBytes;
            *usLen += 1;

            eRegStatus =
                __eMBRegDiscreteCB(regs, pucFrameCur, usRegAddress, usDiscreteCnt );

            /* If an error occured convert it into a Modbus exception. */
            if( eRegStatus != MB_ENOERR ){
                eStatus = prveMBError2Exception( eRegStatus );
            }
            else{
                /* The response contains the function code, the starting address
                 * and the quantity of registers. We reuse the old values in the 
                 * buffer because they are still valid. */
                *usLen += ucNBytes;;
            }
        }
        else{
            eStatus = MB_EX_ILLEGAL_DATA_VALUE;
        }
    }
    else{
        /* Can't be a valid read coil register request because the length
         * is incorrect. */
        eStatus = MB_EX_ILLEGAL_DATA_VALUE;
    }
    
    return eStatus;
}
#endif

#endif

/*************************************************************************************************/
/* TODO implement modbus master */
#if MB_MASTER_ENABLE > 0
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
