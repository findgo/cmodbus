

#include "mbfunccb.h"

#define BITS_uint8_t      8U

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
 * xMBUtilSetBits( ucBits, 4, 1, 1 );
 *
 * // Set bit 7 to 1 and bit 8 to 0.
 * xMBUtilSetBits( ucBits, 7, 2, 0x01 );
 *
 * // Set bits 8 - 11 to 0x05 and bits 12 - 15 to 0x0A;
 * xMBUtilSetBits( ucBits, 8, 8, 0x5A);
 * \endcode
 */
void xMBUtilSetBits( uint8_t * ucByteBuf, uint16_t usBitOffset, uint8_t ucNBits,
                uint8_t ucValue )
{
    uint16_t          usWordBuf;
    uint16_t          usMask;
    uint16_t          usByteOffset;
    uint16_t          usNPreBits;
    uint16_t          usValue = ucValue;

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
 * ucResult = xMBUtilGetBits( ucBits, 3, 8 );
 * \endcode
 */
uint8_t xMBUtilGetBits( uint8_t * ucByteBuf, uint16_t usBitOffset, uint8_t ucNBits )
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


/**
  * @brief  ��Ȧ�Ĵ�������������Ȧ�Ĵ����ɶ����ɶ���д
  * @param  pucRegBuffer  ������---��������ָ�룬д����--��������ָ��
  *         usAddress     �Ĵ�����ʼ��ַ
  *         usNRegs       �Ĵ�������
  *         eMode         ������ʽ��������д
  * @retval eStatus       �Ĵ���״̬
  */
  /* ���ݶ��壬����ֻ������Ȧ״̬����*/
eMBErrorCode eMBRegCoilsCB(mb_reg_t *regs,uint8_t * pucRegBuffer, uint16_t usAddress, uint16_t usNCoils,eMBRegisterMode eMode )
{
    int16_t         iNCoils;
    uint16_t        usBitOffset;

    if( ((int16_t)usAddress >= regs->reg_coils_addr_start) && \
        ( usAddress + usNCoils <= regs->reg_coils_addr_start + regs->reg_coils_num ) ){

        //�����ַƫ����
        iNCoils 	= ( int16_t )usNCoils;
        usBitOffset = ( uint16_t )( usAddress - regs->reg_coils_addr_start );
        switch ( eMode ){
        case MB_REG_READ:
            while( iNCoils > 0 )
            {
                *pucRegBuffer++ = xMBUtilGetBits( regs->pRegCoil, usBitOffset,( uint8_t )( iNCoils > 8 ? 8 : iNCoils ) );
                iNCoils -= 8;
                usBitOffset += 8;
            }
            break;

        //д����
        case MB_REG_WRITE:
            while( iNCoils > 0 )
            {
                xMBUtilSetBits(regs->pRegCoil, usBitOffset,( uint8_t )( iNCoils > 8 ? 8 : iNCoils ),*pucRegBuffer++);
                iNCoils -= 8;
                usBitOffset+=8;
            }
            break;
        }
        
        return MB_ENOERR;
    }
        
    return MB_ENOREG;
}

eMBErrorCode eMBRegDiscreteCB(mb_reg_t *regs, uint8_t * pucRegBuffer, uint16_t usAddress, uint16_t usNDiscrete )
{
  int16_t         iNDiscrete = ( uint16_t )usNDiscrete;
  uint16_t        usBitOffset;

  //�жϼĴ���ʱ�����ƶ���Χ��
    if( ((int16_t)usAddress >= regs->reg_discrete_addr_start) \
        && (usAddress + usNDiscrete <= regs->reg_discrete_addr_start + regs->reg_discrete_num)){

        usBitOffset = ( uint16_t )(usAddress - regs->reg_discrete_addr_start);
        while( iNDiscrete > 0 )
        {
            *pucRegBuffer++ = xMBUtilGetBits( regs->pRegDisc, usBitOffset,( uint8_t)( iNDiscrete > 8 ? 8 : iNDiscrete ) );
            iNDiscrete -= 8;
            usBitOffset += 8;
        }
        
        return MB_ENOERR;
    }

    return MB_ENOREG;
}


/**
  * @brief  ���ּĴ��������������ּĴ����ɶ����ɶ���д
  * @param  pucRegBuffer  ������ʱ--��������ָ�룬д����ʱ--��������ָ��
  *         usAddress     �Ĵ�����ʼ��ַ
  *         usNRegs       �Ĵ�������
  *         eMode         ������ʽ��������д
  * @retval eStatus       �Ĵ���״̬
  */
eMBErrorCode  eMBRegHoldingCB(mb_reg_t *regs, uint8_t * pucRegBuffer, uint16_t usAddress, uint16_t usNRegs,eMBRegisterMode eMode )
{
    int16_t         iRegIndex;
  
    if( ((int16_t)usAddress >= regs->reg_holding_addr_start) \
        && ((usAddress + usNRegs) <= (regs->reg_holding_addr_start + regs->reg_holding_num)) ){
        
        //����ƫ����
        iRegIndex = ( int16_t )( usAddress - regs->reg_holding_addr_start);
        switch ( eMode ){
        case MB_REG_READ:
            while( usNRegs > 0 )
            {
                *pucRegBuffer++ = (uint8_t)( regs->pReghold[iRegIndex] >> 8 );
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
  * @brief  ����Ĵ���������������Ĵ����ɶ���������д��
  * @param  pucRegBuffer  ��������ָ��
  *         usAddress     �Ĵ�����ʼ��ַ
  *         usNRegs       �Ĵ�������
  * @retval eStatus       �Ĵ���״̬
  */

eMBErrorCode  eMBRegInputCB(mb_reg_t *regs, uint8_t * pucRegBuffer, uint16_t usAddress, uint16_t usNRegs )
{
    int16_t         iRegIndex;
  
    if((( int16_t ) usAddress >= regs->reg_input_addr_start ) \
        && ( usAddress + usNRegs) <= regs->reg_input_addr_start + regs->reg_input_num ){
        
        //��ò���ƫ���������β�����ʼ��ַ-����Ĵ����ĳ�ʼ��ַ
        iRegIndex = ( int16_t )( usAddress - regs->reg_input_addr_start);

        while( usNRegs > 0 )
        {
            //��ֵ���ֽ�
            *pucRegBuffer++ = ( uint8_t )( regs->pReginput[iRegIndex] >> 8 );
            //��ֵ���ֽ�
            *pucRegBuffer++ = ( uint8_t )( regs->pReginput[iRegIndex] & 0xFF );
            //ƫ��������
            iRegIndex++;
            //�������Ĵ��������ݼ�
            usNRegs--;
        }
        return MB_ENOERR;
    }

    return MB_ENOREG;
}

