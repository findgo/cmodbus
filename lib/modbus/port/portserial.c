
#include "port.h"
#include "mbrtu.h"
#include "mbascii.h"
#include "modbus.h"

#if MB_RTU_ENABLED > 0 ||  MB_ASCII_ENABLED > 0

//STM32操作相关头文件
#include "stm32f10x.h"
#include "stm32f10x_it.h"

extern Mbs_Device_t *device0;
extern Mbs_Device_t *device1;
extern Mbm_Device_t *deviceM0;
extern Mbm_Device_t *deviceM1;

/* ----------------------- Start implementation -----------------------------*/
/**
  * @brief  控制接收和发送状态
  * @param  xRxEnable 接收使能、
  *         xTxEnable 发送使能
  * @retval None
  */
void vMBPortSerialEnable(uint8_t port, bool xRxEnable, bool xTxEnable)
{
    switch(port){
    case MBCOM0:
        if(xRxEnable){
            //使能接收和接收中断
            USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
            //MAX485操作 低电平为接收模式
            GPIO_ResetBits(GPIOD,GPIO_Pin_8);
        }
        else{
            USART_ITConfig(USART1, USART_IT_RXNE, DISABLE); 
            //MAX485操作 高电平为发送模式
            GPIO_SetBits(GPIOD,GPIO_Pin_8);
        }

        if(xTxEnable){
            //使能发送完成中断
            USART_ITConfig(USART1, USART_IT_TC, ENABLE);
        }
        else{
            //禁止发送完成中断
            USART_ITConfig(USART1, USART_IT_TC, DISABLE);
        }
        break;
        
     case MBCOM1:
        if(xRxEnable){
            //使能接收和接收中断
            USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
            //MAX485操作 低电平为接收模式
            GPIO_ResetBits(GPIOD,GPIO_Pin_9);
        }
        else{
            USART_ITConfig(USART2, USART_IT_RXNE, DISABLE); 
            //MAX485操作 高电平为发送模式
            GPIO_SetBits(GPIOD,GPIO_Pin_9);
        }

        if(xTxEnable){
            //使能发送完成中断
            USART_ITConfig(USART2, USART_IT_TC, ENABLE);
        }
        else{
            //禁止发送完成中断
            USART_ITConfig(USART2, USART_IT_TC, DISABLE);
        }
        break;
        
     default:
        break;
    }
}

/**
  * @brief  串口初始化
  * @param  ucPORT      串口号
  *         ulBaudRate  波特率
  *         ucDataBits  数据位
  *         eParity     校验位 
  * @retval None
  */
bool xMBPortSerialInit(uint8_t ucPORT, uint32_t ulBaudRate, uint8_t ucDataBits, mb_Parity_t eParity)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;  

    (void)ucDataBits; //不修改数据位长度
    (void)eParity;    //不修改校验格式
    
    switch (ucPORT){
    case MBCOM0:
        //使能USART1，GPIOA
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);
        
        //GPIOA9 USART1_Tx
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;             //推挽输出
        GPIO_Init(GPIOA, &GPIO_InitStructure);
        //GPIOA.10 USART1_Rx
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;       //浮动输入
        GPIO_Init(GPIOA, &GPIO_InitStructure);
        
        USART_InitStructure.USART_BaudRate = ulBaudRate;            //只修改波特率
        USART_InitStructure.USART_WordLength = USART_WordLength_8b;
        USART_InitStructure.USART_StopBits = USART_StopBits_1;
        USART_InitStructure.USART_Parity = USART_Parity_No;
        USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
        USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
        //串口初始化
        USART_Init(USART1, &USART_InitStructure);
        //使能USART1
        USART_Cmd(USART1, ENABLE);
        
        //设定USART1 中断优先级
        NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&NVIC_InitStructure);
        
        //最后配置485发送和接收模式
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
        //GPIOD.8
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8; 
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
        GPIO_Init(GPIOD, &GPIO_InitStructure); 
        break;
        
     case MBCOM1:
        //使能USART2，GPIOA
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
        //GPIOA2 USART2_Tx
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;             //推挽输出
        GPIO_Init(GPIOA, &GPIO_InitStructure);
        //GPIOA3 USART2_Rx
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;       //浮动输入
        GPIO_Init(GPIOA, &GPIO_InitStructure);
        
        USART_InitStructure.USART_BaudRate = ulBaudRate;            //只修改波特率
        USART_InitStructure.USART_WordLength = USART_WordLength_8b;
        USART_InitStructure.USART_StopBits = USART_StopBits_1;
        USART_InitStructure.USART_Parity = USART_Parity_No;
        USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
        USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
        //串口初始化
        USART_Init(USART2, &USART_InitStructure);
        //使能USART1
        USART_Cmd(USART2, ENABLE);
        
        //设定USART1 中断优先级
        NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&NVIC_InitStructure);
        
        //最后配置485发送和接收模式
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
        //GPIOD.8
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; 
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
        GPIO_Init(GPIOD, &GPIO_InitStructure); 

        break;
        
     default:
        return FALSE;
    }

  return TRUE;
}

/**
  * @brief  通过串口发送数据
  * @param  None
  * @retval None
  */
bool xMBPortSerialPutByte(uint8_t port, char ucByte )
{
    switch (port){
    case MBCOM0:
        USART_SendData(USART1, ucByte);
        break;
    
    case MBCOM1:
        USART_SendData(USART2, ucByte);
        break;
    
    default:
        break;
    }
  
  return TRUE;
}

/**
  * @brief  从串口获得数据
  * @param  None
  * @retval None
  */
bool xMBPortSerialGetByte(uint8_t port, char *pucByte )
{
    switch (port){
    case MBCOM0:    
        *pucByte = USART_ReceiveData(USART1);
        break;
    
    case MBCOM1:
        *pucByte = USART_ReceiveData(USART2);
        break;
    
    default:
        break;
    }
    
    return TRUE;
}
/**
  * @brief  USART1中断服务函数
  * @param  None
  * @retval None
  */
void USART1_IRQHandler(void)
{
  //发生接收中断
    if(USART_GetITStatus(USART1, USART_IT_RXNE) == SET){
        
#if MB_MASTER_ENABLED > 0
#if MB_RTU_ENABLED > 0
                vMBMRTUReceiveFSM(deviceM0);
#endif
#if MB_ASCII_ENABLED > 0
                vMBMASCIIReceiveFSM(deviceM0);
#endif
#endif
    
#if MB_SLAVE_ENABLED > 0
#if MB_RTU_ENABLED > 0
        vMbsRTUReceiveFSM(device0);
#endif
#if MB_ASCII_ENABLED > 0
        vMbsASCIIReceiveFSM(device0);
#endif
#endif
        //清除中断标志位    
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);   
    }
  
  //发生完成中断
    if(USART_GetITStatus(USART1, USART_IT_TC) == SET){

#if MB_MASTER_ENABLED > 0
#if MB_RTU_ENABLED > 0
        vMBMRTUTransmitFSM(deviceM0);
#endif
#if MB_ASCII_ENABLED > 0
        vMBMASCIITransmitFSM(deviceM0);
#endif
#endif

#if MB_SLAVE_ENABLED > 0
#if MB_RTU_ENABLED > 0
        vMbsRTUTransmitFSM(device0);
#endif
#if MB_ASCII_ENABLED > 0
        vMbsASCIITransmitFSM(device0);
#endif
#endif
        USART_ClearITPendingBit(USART1, USART_IT_TC);
    }
  
  //测试看是否可以去除 2012-07-23
  //溢出-如果发生溢出需要先读SR,再读DR寄存器 则可清除不断入中断的问题
  /*
  if(USART_GetFlagStatus(USART1,USART_FLAG_ORE)==SET)
  {
    USART_ClearFlag(USART1,USART_FLAG_ORE); //读SR
    USART_ReceiveData(USART1);              //读DR
  }
  */
}
/**
  * @brief  USART2中断服务函数
  * @param  None
  * @retval None
  */
void USART2_IRQHandler(void)
{
  //发生接收中断
    if(USART_GetITStatus(USART2, USART_IT_RXNE) == SET){
#if MB_SLAVE_ENABLED > 0
#if MB_RTU_ENABLED > 0
        vMbsRTUReceiveFSM(device1);
#endif
#if MB_ASCII_ENABLED > 0
        vMbsASCIIReceiveFSM(device1);
#endif
#endif

#if MB_MASTER_ENABLED > 0
#if MB_RTU_ENABLED > 0
        vMBMRTUReceiveFSM(deviceM1);
#endif
#if MB_ASCII_ENABLED > 0
        vMBMASCIIReceiveFSM(deviceM1);
#endif
#endif
        //清除中断标志位    
        USART_ClearITPendingBit(USART2, USART_IT_RXNE);   
    }
  
  //发生完成中断
    if(USART_GetITStatus(USART2, USART_IT_TC) == SET){
#if MB_SLAVE_ENABLED > 0
#if MB_RTU_ENABLED > 0
        vMbsRTUTransmitFSM(device1);
#endif
#if MB_ASCII_ENABLED > 0
        vMbsASCIITransmitFSM(device1);
#endif
#endif
#if MB_MASTER_ENABLED > 0
#if MB_RTU_ENABLED > 0
        vMBMRTUTransmitFSM(deviceM1);
#endif
#if MB_ASCII_ENABLED > 0
        vMBMASCIITransmitFSM(deviceM1);
#endif
#endif

        USART_ClearITPendingBit(USART2, USART_IT_TC);
    }
  
  //测试看是否可以去除 2012-07-23
  //溢出-如果发生溢出需要先读SR,再读DR寄存器 则可清除不断入中断的问题
  /*
  if(USART_GetFlagStatus(USART2,USART_FLAG_ORE)==SET)
  {
    USART_ClearFlag(USART2,USART_FLAG_ORE); //读SR
    USART_ReceiveData(USART2);              //读DR
  }
  */
}

#endif

