#include "port.h"
#include "modbus.h"
//STM32相关头文件
#include "stm32f10x.h"
#include "stm32f10x_it.h"


extern mb_device_t device1;

/* ----------------------- Start implementation -----------------------------*/
/**
  * @brief  定时器初始化函数
  * @param  None
  * @retval None
  */
bool xMBPortTimersInit(uint8_t port, uint16_t usTim1Timerout50us )
{
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    //
    uint16_t PrescalerValue = 0;

    //使能定时器4时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

    //定时器时间基配置说明
    //HCLK为72MHz，APB1经过2分频为36MHz
    //TIM4的时钟倍频后为72MHz（硬件自动倍频,达到最大）
    //TIM4的分频系数为3599，时间基频率为72 / (1 + Prescaler) = 20KHz,基准为50us
    //TIM最大计数值为usTim1Timerout50u
    PrescalerValue = (uint16_t) (SystemCoreClock / 20000) - 1; 
    //定时器1初始化
    TIM_TimeBaseStructure.TIM_Period = (uint16_t) usTim1Timerout50us;
    TIM_TimeBaseStructure.TIM_Prescaler = PrescalerValue;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);
    //预装载使能
    TIM_ARRPreloadConfig(TIM4, ENABLE);

    //
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    //定时器4中断优先级
    NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    //清除溢出中断标志位
    TIM_ClearITPendingBit(TIM4,TIM_IT_Update);
    //定时器4溢出中断关闭
    TIM_ITConfig(TIM4, TIM_IT_Update, DISABLE);
    //定时器4禁能
    TIM_Cmd(TIM4,  DISABLE);
    
    return true;
}


void vMBPortTimersEnable(uint8_t port  )
{
    /* Enable the timer with the timeout passed to xMBPortTimersInit( ) */

    TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
    TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);
    //设定定时器4的初始值
    TIM_SetCounter(TIM4,0x0000); 
    //定时器4启动
    TIM_Cmd(TIM4, ENABLE);
}

void vMBPortTimersDisable(uint8_t port  )
{
    /* Disable any pending timers. */
    TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
    TIM_ITConfig(TIM4, TIM_IT_Update, DISABLE);
    TIM_SetCounter(TIM4,0x0000); 
    //关闭定时器4
    TIM_Cmd(TIM4, DISABLE);
}

/**
  * @brief  定时器4中断服务函数
  * @param  None
  * @retval None
  */
void TIM4_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET){
        //清除定时器T4溢出中断标志位
        TIM_ClearITPendingBit(TIM4, TIM_IT_Update);

        xMBRTUTimerT35Expired(&device1);
    }
}
