

#include "mb.h"
#include "mbconfig.h"
#include "mbutils.h"


//����Ĵ�����ʼ��ַ
#define REG_INPUT_START       0x0000
//���ּĴ�����ʼ��ַ
#define REG_HOLDING_START     0x0000
//��ɢ��������ʼ��ַ
#define REG_DISCRETE_START    0x0000
//��Ȧ��ʼ��ַ
#define REG_COILS_START       0x0000


/* Private variables ---------------------------------------------------------*/

/*only read*/
#if MB_FUNC_READ_INPUT_ENABLED > 0
//����Ĵ�������
uint16_t usRegInputBuf[REG_INPUT_NREGS];
#endif

#if (MB_FUNC_WRITE_HOLDING_ENABLED > 0)|| (MB_FUNC_WRITE_MULTIPLE_HOLDING_ENABLED > 0)||  \
	(MB_FUNC_READ_HOLDING_ENABLED > 0)||(MB_FUNC_READWRITE_HOLDING_ENABLED > 0)||(MB_FUNC_READWRITE_HOLDING_ENABLED > 0)
//���ּĴ�������
uint16_t usRegHoldingBuf[REG_HOLDING_NREGS];
#endif

#if MB_FUNC_READ_DISCRETE_INPUTS_ENABLED > 0
//��ɢ����״̬
/*only read*/
uint8_t ucRegDiscreteBuf[REG_DISCRETE_SIZE / 8];
#endif


#if (MB_FUNC_WRITE_MULTIPLE_COILS_ENABLED > 0)||(MB_FUNC_WRITE_COIL_ENABLED > 0)||(MB_FUNC_READ_COILS_ENABLED > 0)
//��Ȧ״̬
uint8_t ucRegCoilsBuf[REG_COILS_SIZE / 8];
#endif

//�Ĵ�����ʼ��ַ
//static uint16_t usRegInputStart = REG_INPUT_START;
//���ּĴ�����ʼ��ַ
//static uint16_t usRegHoldingStart = REG_HOLDING_START;


/**
  * @brief  ����Ĵ���������������Ĵ����ɶ���������д��
  * @param  pucRegBuffer  ��������ָ��
  *         usAddress     �Ĵ�����ʼ��ַ
  *         usNRegs       �Ĵ�������
  * @retval eStatus       �Ĵ���״̬
  */

#if MB_FUNC_READ_INPUT_ENABLED > 0
eMBErrorCode  eMBRegInputCB( uint8_t * pucRegBuffer, uint16_t usAddress, uint16_t usNRegs )
{
  eMBErrorCode    eStatus = MB_ENOERR;
  int16_t         iRegIndex;
  
  //��ѯ�Ƿ��ڼĴ�����Χ��
  //Ϊ�˱��⾯�棬�޸�Ϊ�з�������
  if( (( int16_t ) usAddress >= REG_INPUT_START ) && ( usAddress + usNRegs) <= REG_INPUT_START + REG_INPUT_NREGS )
  {
    //��ò���ƫ���������β�����ʼ��ַ-����Ĵ����ĳ�ʼ��ַ
    iRegIndex = ( int16_t )( usAddress - REG_INPUT_START);
    //�����ֵ
    while( usNRegs > 0 )
    {
      //��ֵ���ֽ�
      *pucRegBuffer++ = ( uint8_t )( usRegInputBuf[iRegIndex] >> 8 );
      //��ֵ���ֽ�
      *pucRegBuffer++ = ( uint8_t )( usRegInputBuf[iRegIndex] & 0xFF );
      //ƫ��������
      iRegIndex++;
      //�������Ĵ��������ݼ�
      usNRegs--;
    }
  }
  else
  {
    //���ش���״̬���޼Ĵ���  
    eStatus = MB_ENOREG;
  }

  return eStatus;
}

#endif
/**
  * @brief  ���ּĴ��������������ּĴ����ɶ����ɶ���д
  * @param  pucRegBuffer  ������ʱ--��������ָ�룬д����ʱ--��������ָ��
  *         usAddress     �Ĵ�����ʼ��ַ
  *         usNRegs       �Ĵ�������
  *         eMode         ������ʽ��������д
  * @retval eStatus       �Ĵ���״̬
  */
#if (MB_FUNC_WRITE_HOLDING_ENABLED > 0)|| (MB_FUNC_WRITE_MULTIPLE_HOLDING_ENABLED > 0)||  \
	(MB_FUNC_READ_HOLDING_ENABLED > 0)||(MB_FUNC_READWRITE_HOLDING_ENABLED > 0)||(MB_FUNC_READWRITE_HOLDING_ENABLED > 0)
eMBErrorCode  eMBRegHoldingCB( uint8_t * pucRegBuffer, uint16_t usAddress, uint16_t usNRegs,eMBRegisterMode eMode )
{
  //����״̬
  eMBErrorCode    eStatus = MB_ENOERR;
  //ƫ����
  int16_t         iRegIndex;
  
  //�жϼĴ����ǲ����ڷ�Χ��
  if( ( (int16_t)usAddress >= REG_HOLDING_START ) && ( (usAddress + usNRegs) <= REG_HOLDING_START + REG_HOLDING_NREGS ) )
  {
    //����ƫ����
    iRegIndex = ( int16_t )( usAddress - REG_HOLDING_START);
    switch ( eMode )
    {
      //��������  
      case MB_REG_READ:
        while( usNRegs > 0 )
        {
          *pucRegBuffer++ = ( uint8_t )( usRegHoldingBuf[iRegIndex] >> 8 );
          *pucRegBuffer++ = ( uint8_t )( usRegHoldingBuf[iRegIndex] & 0xFF );
          iRegIndex++;
          usNRegs--;
        }
        break;

      //д������ 
      case MB_REG_WRITE:
        while( usNRegs > 0 )
        {
          usRegHoldingBuf[iRegIndex] = *pucRegBuffer++ << 8;
          usRegHoldingBuf[iRegIndex] |= *pucRegBuffer++;
          iRegIndex++;
          usNRegs--;
        }
        break;
     }
  }
  else
  {
    //���ش���״̬
    eStatus = MB_ENOREG;
  }
  
  return eStatus;
}
#endif

/**
  * @brief  ��Ȧ�Ĵ�������������Ȧ�Ĵ����ɶ����ɶ���д
  * @param  pucRegBuffer  ������---��������ָ�룬д����--��������ָ��
  *         usAddress     �Ĵ�����ʼ��ַ
  *         usNRegs       �Ĵ�������
  *         eMode         ������ʽ��������д
  * @retval eStatus       �Ĵ���״̬
  */
  /* ���ݶ��壬����ֻ������Ȧ״̬����*/

#if (MB_FUNC_WRITE_MULTIPLE_COILS_ENABLED > 0)||(MB_FUNC_WRITE_COIL_ENABLED > 0)||(MB_FUNC_READ_COILS_ENABLED > 0)
eMBErrorCode eMBRegCoilsCB( uint8_t * pucRegBuffer, uint16_t usAddress, uint16_t usNCoils,eMBRegisterMode eMode )
{
  //����״̬
  	eMBErrorCode    eStatus = MB_ENOERR;
  //�Ĵ�������
	int16_t         iNCoils;
  //�Ĵ���ƫ����
	uint16_t        usBitOffset;

  //����ȡ�����Ƿ���ָ����ַ��Χ��
  if( ( (int16_t)usAddress >= REG_COILS_START ) &&( usAddress + usNCoils <= REG_COILS_START + REG_COILS_SIZE ) )
  {
  
    //�����ַƫ����
    iNCoils 	= ( int16_t )usNCoils;
    usBitOffset = ( uint16_t )( usAddress - REG_COILS_START );
    switch ( eMode )
    {
      //������
      case MB_REG_READ:
        while( iNCoils > 0 )
        {
          	*pucRegBuffer++ = xMBUtilGetBits( ucRegCoilsBuf, usBitOffset,( uint8_t )( iNCoils > 8 ? 8 : iNCoils ) );
	    	iNCoils -= 8;
     	    usBitOffset += 8;
        }
        break;

      //д����
      case MB_REG_WRITE:
        while( iNCoils > 0 )
        {
          xMBUtilSetBits( ucRegCoilsBuf, usBitOffset,( uint8_t )( iNCoils > 8 ? 8 : iNCoils ),*pucRegBuffer++);
  	    	iNCoils -= 8;
	    	usBitOffset+=8;
        }
        break;
    }

  }
  else
  {
    eStatus = MB_ENOREG;
  }
  
  return eStatus;
}

#endif


#if MB_FUNC_READ_DISCRETE_INPUTS_ENABLED > 0
eMBErrorCode eMBRegDiscreteCB( uint8_t * pucRegBuffer, uint16_t usAddress, uint16_t usNDiscrete )
{
  //����״̬
  eMBErrorCode    eStatus = MB_ENOERR;
  //�����Ĵ�������
  int16_t         iNDiscrete = ( uint16_t )usNDiscrete;
  //ƫ����
  uint16_t        usBitOffset;

  //�жϼĴ���ʱ�����ƶ���Χ��
  if( ( (int16_t)usAddress >= REG_DISCRETE_START ) &&( usAddress + usNDiscrete <= REG_DISCRETE_START + REG_DISCRETE_SIZE ) )
  {
    //���ƫ����
    usBitOffset = ( uint16_t )( usAddress - REG_DISCRETE_START );
    
    while( iNDiscrete > 0 )
    {
      	*pucRegBuffer++ = xMBUtilGetBits( ucRegDiscreteBuf, usBitOffset,( uint8_t)( iNDiscrete > 8 ? 8 : iNDiscrete ) );
		iNDiscrete -= 8;
		usBitOffset += 8;
    }
  }
  else
  {
    eStatus = MB_ENOREG;
  }
  return eStatus;
}

#endif

