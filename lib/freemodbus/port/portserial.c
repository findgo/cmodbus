
#include "port.h"
#include "modbus.h"
#include "mbrtu.h"
#include "mbascii.h"

//STM32�������ͷ�ļ�
#include "stm32f10x.h"
#include "stm32f10x_it.h"

extern mb_Device_t device1;

/* ----------------------- Start implementation -----------------------------*/
/**
  * @brief  ���ƽ��պͷ���״̬
  * @param  xRxEnable ����ʹ�ܡ�
  *         xTxEnable ����ʹ��
  * @retval None
  */
void vMBPortSerialEnable(uint8_t port, bool xRxEnable, bool xTxEnable)
{

    (void)port;

    if(xRxEnable){
        //ʹ�ܽ��պͽ����ж�
        USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
        //MAX485���� �͵�ƽΪ����ģʽ
        GPIO_ResetBits(GPIOD,GPIO_Pin_8);
    }
    else{
        USART_ITConfig(USART1, USART_IT_RXNE, DISABLE); 
        //MAX485���� �ߵ�ƽΪ����ģʽ
        GPIO_SetBits(GPIOD,GPIO_Pin_8);
    }

    if(xTxEnable){
        //ʹ�ܷ�������ж�
        USART_ITConfig(USART1, USART_IT_TC, ENABLE);
    }
    else{
        //��ֹ��������ж�
        USART_ITConfig(USART1, USART_IT_TC, DISABLE);
    }

}

/**
  * @brief  ���ڳ�ʼ��
  * @param  ucPORT      ���ں�
  *         ulBaudRate  ������
  *         ucDataBits  ����λ
  *         eParity     У��λ 
  * @retval None
  */
bool xMBPortSerialInit(uint8_t ucPORT, uint32_t ulBaudRate, uint8_t ucDataBits, mb_Parity_t eParity)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  USART_InitTypeDef USART_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;  
    
  (void)ucPORT;     //���޸Ĵ���
  (void)ucDataBits; //���޸�����λ����
  (void)eParity;    //���޸�У���ʽ
  
  //ʹ��USART1��GPIOA
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | 
            RCC_APB2Periph_USART1, ENABLE);

  //GPIOA9 USART1_Tx
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;             //�������
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  //GPIOA.10 USART1_Rx
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;       //��������
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  USART_InitStructure.USART_BaudRate = ulBaudRate;            //ֻ�޸Ĳ�����
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  //���ڳ�ʼ��
  USART_Init(USART1, &USART_InitStructure);
  //ʹ��USART1
  USART_Cmd(USART1, ENABLE);
  
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
  //�趨USART1 �ж����ȼ�
  NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  
  //�������485���ͺͽ���ģʽ
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
  //GPIOD.8
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8; 
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOD, &GPIO_InitStructure); 

  return true;
}

/**
  * @brief  ͨ�����ڷ�������
  * @param  None
  * @retval None
  */
bool xMBPortSerialPutByte(uint8_t port, char ucByte )
{
  (void)port;
  
  //��������
  USART_SendData(USART1, ucByte);
  
  return true;
}

/**
  * @brief  �Ӵ��ڻ������
  * @param  None
  * @retval None
  */
bool xMBPortSerialGetByte(uint8_t port, char *pucByte )
{
    (void)port;
    
    *pucByte = USART_ReceiveData(USART1);
    
    return true;
}

/* Create an interrupt handler for the transmit buffer empty interrupt
 * (or an equivalent) for your target processor. This function should then
 * call pxMBFrameCBTransmitterEmpty( ) which tells the protocol stack that
 * a new character can be sent. The protocol stack will then call 
 * xMBPortSerialPutByte( ) to send the character.
 */


/* Create an interrupt handler for the receive interrupt for your target
 * processor. This function should then call pxMBFrameCBByteReceived( ). The
 * protocol stack will then call xMBPortSerialGetByte( ) to retrieve the
 * character.
 */
 
/**
  * @brief  USART1�жϷ�����
  * @param  None
  * @retval None
  */
void USART1_IRQHandler(void)
{
  //���������ж�
    if(USART_GetITStatus(USART1, USART_IT_RXNE) == SET){
#if MB_RTU_ENABLED > 0
        xMBRTUReceiveFSM(&device1);
#endif
#if MB_ASCII_ENABLED > 0
        xMBASCIIReceiveFSM(&device1);
#endif
        //����жϱ�־λ    
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);   
    }
  
  //��������ж�
    if(USART_GetITStatus(USART1, USART_IT_TC) == SET){
        //mb.c eMBInit������

        //����״̬��
#if MB_RTU_ENABLED > 0
        xMBRTUTransmitFSM(&device1);
#endif
#if MB_ASCII_ENABLED > 0
        xMBASCIITransmitFSM(&device1);
#endif
        //����жϱ�־
        USART_ClearITPendingBit(USART1, USART_IT_TC);
    }
  
  //���Կ��Ƿ����ȥ�� 2012-07-23
  //���-������������Ҫ�ȶ�SR,�ٶ�DR�Ĵ��� �������������жϵ�����
  /*
  if(USART_GetFlagStatus(USART1,USART_FLAG_ORE)==SET)
  {
    USART_ClearFlag(USART1,USART_FLAG_ORE); //��SR
    USART_ReceiveData(USART1);              //��DR
  }
  */
}
