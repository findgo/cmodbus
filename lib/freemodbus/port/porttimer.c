#include "port.h"
#include "modbus.h"
#include "mbrtu.h"
#include "mbascii.h"

//STM32���ͷ�ļ�
#include "stm32f10x.h"
#include "stm32f10x_it.h"


extern mb_Device_t device1;

/* ----------------------- Start implementation -----------------------------*/
/**
  * @brief  ��ʱ����ʼ������
  * @param  None
  * @retval None
  */
bool xMBPortTimersInit(uint8_t port, uint16_t usTim1Timerout50us)
{
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    uint16_t PrescalerValue = 0;

    (void)port;    

    //ʹ�ܶ�ʱ��4ʱ��
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

    //��ʱ��ʱ�������˵��
    //HCLKΪ72MHz��APB1����2��ƵΪ36MHz
    //TIM4��ʱ�ӱ�Ƶ��Ϊ72MHz��Ӳ���Զ���Ƶ,�ﵽ���
    //TIM4�ķ�Ƶϵ��Ϊ3599��ʱ���Ƶ��Ϊ72 / (1 + Prescaler) = 20KHz,��׼Ϊ50us
    //TIM������ֵΪusTim1Timerout50u
    PrescalerValue = (uint16_t) (SystemCoreClock / 20000) - 1; 
    //��ʱ��1��ʼ��
    TIM_TimeBaseStructure.TIM_Period = (uint16_t) usTim1Timerout50us;
    TIM_TimeBaseStructure.TIM_Prescaler = PrescalerValue;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);
    //Ԥװ��ʹ��
    TIM_ARRPreloadConfig(TIM4, ENABLE);

    //
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    //��ʱ��4�ж����ȼ�
    NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    //�������жϱ�־λ
    TIM_ClearITPendingBit(TIM4,TIM_IT_Update);
    //��ʱ��4����жϹر�
    TIM_ITConfig(TIM4, TIM_IT_Update, DISABLE);
    //��ʱ��4����
    TIM_Cmd(TIM4,  DISABLE);
    
    return true;
}


void vMBPortTimersEnable(uint8_t port)
{
    (void)port;
    
    /* Enable the timer with the timeout passed to xMBPortTimersInit( ) */
    TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
    TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);
    //�趨��ʱ��4�ĳ�ʼֵ
    TIM_SetCounter(TIM4,0x0000); 
    //��ʱ��4����
    TIM_Cmd(TIM4, ENABLE);
}

void vMBPortTimersDisable(uint8_t port)
{
    (void)port;
    
    /* Disable any pending timers. */
    TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
    TIM_ITConfig(TIM4, TIM_IT_Update, DISABLE);
    TIM_SetCounter(TIM4,0x0000); 
    //�رն�ʱ��4
    TIM_Cmd(TIM4, DISABLE);
}

/**
  * @brief  ��ʱ��4�жϷ�����
  * @param  None
  * @retval None
  */
void TIM4_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET){
        //�����ʱ��T4����жϱ�־λ
        TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
#if MB_RTU_ENABLED > 0
        xMBRTUTimerT35Expired(&device1);
#endif
#if MB_ASCII_ENABLED > 0
        xMBASCIITimerT1SExpired(&device1);
#endif        
    }
}


#if MB_MASTER_ENABLE > 0
/*This optional function returns the current time in milliseconds (don't care
  for wraparound, this is only used for time diffs).*/
uint32_t xMBsys_now(void)
{
    return 0;
}
#endif