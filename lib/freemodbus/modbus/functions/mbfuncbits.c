

#include "mbfunc.h"

/* ----------------------- Start implementation -----------------------------*/

#if MB_FUNC_READ_COILS_ENABLED > 0
eMBException eMBFuncReadCoils(mb_reg_t *regs, uint8_t *pPdu, uint16_t *usLen )
{
    uint16_t usRegAddress;
    uint16_t usCoilCount;
    uint8_t  ucNBytes;
    uint8_t  *pucFrameCur;

    eMBException eStatus = MB_EX_NONE;
    eMBErrorCode eRegStatus;

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
            pucFrameCur = &pPdu[MB_PDU_FUNC_OFF];
            *usLen = MB_PDU_FUNC_OFF;

            /* First byte contains the function code. */
            *pucFrameCur++ = MB_FUNC_READ_COILS;
            *usLen += 1;

            /* Test if the quantity of coils is a multiple of 8. If not last
             * byte is only partially field with unused coils set to zero. */
            if( ( usCoilCount & 0x0007 ) != 0 ){
                ucNBytes = (uint8_t)( usCoilCount / 8 + 1 );
            }
            else{
                ucNBytes = (uint8_t)( usCoilCount / 8 );
            }
            *pucFrameCur++ = ucNBytes;
            *usLen += 1;

            eRegStatus =
                eMBRegCoilsCB(regs, pucFrameCur, usRegAddress, usCoilCount,MB_REG_READ );

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

#if MB_FUNC_WRITE_COIL_ENABLED > 0
eMBException eMBFuncWriteCoil(mb_reg_t *regs,uint8_t *pPdu, uint16_t * usLen)
{
    uint16_t usRegAddress;
    uint8_t  ucBuf[2];

    eMBException    eStatus = MB_EX_NONE;
    eMBErrorCode    eRegStatus;

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
                eMBRegCoilsCB(regs,&ucBuf[0], usRegAddress, 1, MB_REG_WRITE );

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
eMBException eMBFuncWriteMultipleCoils(mb_reg_t *regs,uint8_t * pPdu, uint16_t * usLen )
{
    uint16_t usRegAddress;
    uint16_t usCoilCnt;
    uint8_t  ucByteCount;
    uint8_t  ucByteCountVerify;

    eMBException    eStatus = MB_EX_NONE;
    eMBErrorCode    eRegStatus;

    if(*usLen > (MB_PDU_FUNC_WRITE_SIZE + MB_PDU_SIZE_MIN )){
        
        usRegAddress = ( uint16_t )( pPdu[MB_PDU_FUNC_WRITE_MUL_ADDR_OFF] << 8 );
        usRegAddress |= ( uint16_t )( pPdu[MB_PDU_FUNC_WRITE_MUL_ADDR_OFF + 1] );

        usCoilCnt = ( uint16_t )( pPdu[MB_PDU_FUNC_WRITE_MUL_COILCNT_OFF] << 8 );
        usCoilCnt |= ( uint16_t )( pPdu[MB_PDU_FUNC_WRITE_MUL_COILCNT_OFF + 1] );

        ucByteCount = pPdu[MB_PDU_FUNC_WRITE_MUL_BYTECNT_OFF];

        /* Compute the number of expected bytes in the request. */
        if( ( usCoilCnt & 0x0007 ) != 0 ){
            ucByteCountVerify = (uint8_t)( usCoilCnt / 8 + 1 );
        }
        else{
            ucByteCountVerify = (uint8_t)( usCoilCnt / 8 );
        }

        if( (usCoilCnt >= MB_WRITEBITS_CNT_MIN ) \
            && (usCoilCnt <= MB_WRITEBITS_CNT_MAX ) \
             && (ucByteCountVerify == ucByteCount ) ){
             
            eRegStatus =
                eMBRegCoilsCB(regs,&pPdu[MB_PDU_FUNC_WRITE_MUL_VALUES_OFF], usRegAddress, usCoilCnt, MB_REG_WRITE );

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

#endif

#if MB_FUNC_READ_COILS_ENABLED > 0
eMBException eMBFuncReadDiscreteInputs(mb_reg_t *regs, uint8_t * pPdu, uint16_t *usLen )
{
    uint16_t usRegAddress;
    uint16_t usDiscreteCnt;
    uint8_t  ucNBytes;
    uint8_t  *pucFrameCur;

    eMBException eStatus = MB_EX_NONE;
    eMBErrorCode eRegStatus;

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
            pucFrameCur = &pPdu[MB_PDU_FUNC_OFF];
            *usLen = MB_PDU_FUNC_OFF;

            /* First byte contains the function code. */
            *pucFrameCur++ = MB_FUNC_READ_DISCRETE_INPUTS;
            *usLen += 1;

            /* Test if the quantity of coils is a multiple of 8. If not last
             * byte is only partially field with unused coils set to zero. */
            if((usDiscreteCnt & 0x0007) != 0){
                ucNBytes = ( uint8_t ) ( usDiscreteCnt / 8 + 1 );
            }
            else{
                ucNBytes = ( uint8_t ) ( usDiscreteCnt / 8 );
            }
            *pucFrameCur++ = ucNBytes;
            *usLen += 1;

            eRegStatus =
                eMBRegDiscreteCB(regs,pucFrameCur, usRegAddress, usDiscreteCnt );

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

