

#include "mb.h"
#include "mbconfig.h"
#include "mbutils.h"


//输入寄存器起始地址
#define REG_INPUT_START       0x0000
//保持寄存器起始地址
#define REG_HOLDING_START     0x0000
//离散量输入起始地址
#define REG_DISCRETE_START    0x0000
//线圈起始地址
#define REG_COILS_START       0x0000


/* Private variables ---------------------------------------------------------*/

/*only read*/
#if MB_FUNC_READ_INPUT_ENABLED > 0
//输入寄存器内容
uint16_t usRegInputBuf[REG_INPUT_NREGS];
#endif

#if (MB_FUNC_WRITE_HOLDING_ENABLED > 0)|| (MB_FUNC_WRITE_MULTIPLE_HOLDING_ENABLED > 0)||  \
	(MB_FUNC_READ_HOLDING_ENABLED > 0)||(MB_FUNC_READWRITE_HOLDING_ENABLED > 0)||(MB_FUNC_READWRITE_HOLDING_ENABLED > 0)
//保持寄存器内容
uint16_t usRegHoldingBuf[REG_HOLDING_NREGS];
#endif

#if MB_FUNC_READ_DISCRETE_INPUTS_ENABLED > 0
//离散输入状态
/*only read*/
uint8_t ucRegDiscreteBuf[REG_DISCRETE_SIZE / 8];
#endif


#if (MB_FUNC_WRITE_MULTIPLE_COILS_ENABLED > 0)||(MB_FUNC_WRITE_COIL_ENABLED > 0)||(MB_FUNC_READ_COILS_ENABLED > 0)
//线圈状态
uint8_t ucRegCoilsBuf[REG_COILS_SIZE / 8];
#endif

//寄存器起始地址
//static uint16_t usRegInputStart = REG_INPUT_START;
//保持寄存器起始地址
//static uint16_t usRegHoldingStart = REG_HOLDING_START;


/**
  * @brief  输入寄存器处理函数，输入寄存器可读，但不可写。
  * @param  pucRegBuffer  返回数据指针
  *         usAddress     寄存器起始地址
  *         usNRegs       寄存器长度
  * @retval eStatus       寄存器状态
  */

#if MB_FUNC_READ_INPUT_ENABLED > 0
eMBErrorCode  eMBRegInputCB( uint8_t * pucRegBuffer, uint16_t usAddress, uint16_t usNRegs )
{
  eMBErrorCode    eStatus = MB_ENOERR;
  int16_t         iRegIndex;
  
  //查询是否在寄存器范围内
  //为了避免警告，修改为有符号整数
  if( (( int16_t ) usAddress >= REG_INPUT_START ) && ( usAddress + usNRegs) <= REG_INPUT_START + REG_INPUT_NREGS )
  {
    //获得操作偏移量，本次操作起始地址-输入寄存器的初始地址
    iRegIndex = ( int16_t )( usAddress - REG_INPUT_START);
    //逐个赋值
    while( usNRegs > 0 )
    {
      //赋值高字节
      *pucRegBuffer++ = ( uint8_t )( usRegInputBuf[iRegIndex] >> 8 );
      //赋值低字节
      *pucRegBuffer++ = ( uint8_t )( usRegInputBuf[iRegIndex] & 0xFF );
      //偏移量增加
      iRegIndex++;
      //被操作寄存器数量递减
      usNRegs--;
    }
  }
  else
  {
    //返回错误状态，无寄存器  
    eStatus = MB_ENOREG;
  }

  return eStatus;
}

#endif
/**
  * @brief  保持寄存器处理函数，保持寄存器可读，可读可写
  * @param  pucRegBuffer  读操作时--返回数据指针，写操作时--输入数据指针
  *         usAddress     寄存器起始地址
  *         usNRegs       寄存器长度
  *         eMode         操作方式，读或者写
  * @retval eStatus       寄存器状态
  */
#if (MB_FUNC_WRITE_HOLDING_ENABLED > 0)|| (MB_FUNC_WRITE_MULTIPLE_HOLDING_ENABLED > 0)||  \
	(MB_FUNC_READ_HOLDING_ENABLED > 0)||(MB_FUNC_READWRITE_HOLDING_ENABLED > 0)||(MB_FUNC_READWRITE_HOLDING_ENABLED > 0)
eMBErrorCode  eMBRegHoldingCB( uint8_t * pucRegBuffer, uint16_t usAddress, uint16_t usNRegs,eMBRegisterMode eMode )
{
  //错误状态
  eMBErrorCode    eStatus = MB_ENOERR;
  //偏移量
  int16_t         iRegIndex;
  
  //判断寄存器是不是在范围内
  if( ( (int16_t)usAddress >= REG_HOLDING_START ) && ( (usAddress + usNRegs) <= REG_HOLDING_START + REG_HOLDING_NREGS ) )
  {
    //计算偏移量
    iRegIndex = ( int16_t )( usAddress - REG_HOLDING_START);
    switch ( eMode )
    {
      //读处理函数  
      case MB_REG_READ:
        while( usNRegs > 0 )
        {
          *pucRegBuffer++ = ( uint8_t )( usRegHoldingBuf[iRegIndex] >> 8 );
          *pucRegBuffer++ = ( uint8_t )( usRegHoldingBuf[iRegIndex] & 0xFF );
          iRegIndex++;
          usNRegs--;
        }
        break;

      //写处理函数 
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
    //返回错误状态
    eStatus = MB_ENOREG;
  }
  
  return eStatus;
}
#endif

/**
  * @brief  线圈寄存器处理函数，线圈寄存器可读，可读可写
  * @param  pucRegBuffer  读操作---返回数据指针，写操作--返回数据指针
  *         usAddress     寄存器起始地址
  *         usNRegs       寄存器长度
  *         eMode         操作方式，读或者写
  * @retval eStatus       寄存器状态
  */
  /* 根据定义，这里只处理线圈状态数据*/

#if (MB_FUNC_WRITE_MULTIPLE_COILS_ENABLED > 0)||(MB_FUNC_WRITE_COIL_ENABLED > 0)||(MB_FUNC_READ_COILS_ENABLED > 0)
eMBErrorCode eMBRegCoilsCB( uint8_t * pucRegBuffer, uint16_t usAddress, uint16_t usNCoils,eMBRegisterMode eMode )
{
  //错误状态
  	eMBErrorCode    eStatus = MB_ENOERR;
  //寄存器个数
	int16_t         iNCoils;
  //寄存器偏移量
	uint16_t        usBitOffset;

  //检查读取数据是否在指定地址范围内
  if( ( (int16_t)usAddress >= REG_COILS_START ) &&( usAddress + usNCoils <= REG_COILS_START + REG_COILS_SIZE ) )
  {
  
    //计算地址偏移量
    iNCoils 	= ( int16_t )usNCoils;
    usBitOffset = ( uint16_t )( usAddress - REG_COILS_START );
    switch ( eMode )
    {
      //读操作
      case MB_REG_READ:
        while( iNCoils > 0 )
        {
          	*pucRegBuffer++ = xMBUtilGetBits( ucRegCoilsBuf, usBitOffset,( uint8_t )( iNCoils > 8 ? 8 : iNCoils ) );
	    	iNCoils -= 8;
     	    usBitOffset += 8;
        }
        break;

      //写操作
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
  //错误状态
  eMBErrorCode    eStatus = MB_ENOERR;
  //操作寄存器个数
  int16_t         iNDiscrete = ( uint16_t )usNDiscrete;
  //偏移量
  uint16_t        usBitOffset;

  //判断寄存器时候再制定范围内
  if( ( (int16_t)usAddress >= REG_DISCRETE_START ) &&( usAddress + usNDiscrete <= REG_DISCRETE_START + REG_DISCRETE_SIZE ) )
  {
    //获得偏移量
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

