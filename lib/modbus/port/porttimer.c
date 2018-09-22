#include "port.h"
#include "mbrtu.h"
#include "mbascii.h"
#include "modbus.h"

#if MB_RTU_ENABLED > 0 ||  MB_ASCII_ENABLED > 0
//STM32相关头文件
#include "stm32f10x.h"
#include "stm32f10x_it.h"

uint32_t sysclocktime = 0;

extern Mbs_Device_t *device0;
extern Mbs_Device_t *device1;
extern Mbm_Device_t *deviceM0;
extern Mbm_Device_t *deviceM1;
/* ----------------------- Start implementation -----------------------------*/
/**
  * @brief  定时器初始化函数
  * @param  None
  * @retval None
  */
bool xMBPortTimersInit(uint8_t port, uint16_t usTim1Timerout50us)
{
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    uint16_t PrescalerValue = 0;

    switch(port){
    case MBCOM0:
        //使能定时器3时钟
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

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
        TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
        //预装载使能
        TIM_ARRPreloadConfig(TIM3, ENABLE);

        //定时器4中断优先级
        NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&NVIC_InitStructure);

        //清除溢出中断标志位
        TIM_ClearITPendingBit(TIM3,TIM_IT_Update);
        //定时器3溢出中断关闭
        TIM_ITConfig(TIM3, TIM_IT_Update, DISABLE);
        //定时器3禁能
        TIM_Cmd(TIM3,  DISABLE);
        break;
    case MBCOM1:
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
        break;
    default:
        return FALSE;
    }
    
    return TRUE;
}


void vMBPortTimersEnable(uint8_t port)
{
    switch(port){
    case MBCOM0:
        /* Enable the timer with the timeout passed to xMBPortTimersInit( ) */
        TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
        TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
        //设定定时器4的初始值
        TIM_SetCounter(TIM3,0x0000); 
        //定时器4启动
        TIM_Cmd(TIM3, ENABLE);
        break;
    case MBCOM1:
        /* Enable the timer with the timeout passed to xMBPortTimersInit( ) */
        TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
        TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);
        //设定定时器4的初始值
        TIM_SetCounter(TIM4,0x0000); 
        //定时器4启动
        TIM_Cmd(TIM4, ENABLE);
        break;
    default:
        break;
    }
}

void vMBPortTimersDisable(uint8_t port)
{
    switch(port){
    case MBCOM0:
        /* Disable any pending timers. */
        TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
        TIM_ITConfig(TIM3, TIM_IT_Update, DISABLE);
        TIM_SetCounter(TIM3,0x0000); 
        //关闭定时器4
        TIM_Cmd(TIM3, DISABLE);
        break;
    case MBCOM1:
        /* Disable any pending timers. */
        TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
        TIM_ITConfig(TIM4, TIM_IT_Update, DISABLE);
        TIM_SetCounter(TIM4,0x0000); 
        //关闭定时器4
        TIM_Cmd(TIM4, DISABLE);
        break;
    default:
        break;
    }
}
/**
  * @brief  定时器3中断服务函数
  * @param  None
  * @retval None
  */
void TIM3_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET){
        //清除定时器T4溢出中断标志位
        TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
#if MB_SLAVE_ENABLED > 0
#if MB_RTU_ENABLED > 0
        vMbsRTUTimerT35Expired(device0);
#endif
#if MB_ASCII_ENABLED > 0
        vMbsASCIITimerT1SExpired(device0);
#endif        
#endif

#if MB_MASTER_ENABLED > 0
#if MB_RTU_ENABLED > 0
        vMBMRTUTimerT35Expired(deviceM0);
#endif
#if MB_ASCII_ENABLED > 0
        vMBMASCIITimerT1SExpired(deviceM0);
#endif        

#endif
    }
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
#if MB_SLAVE_ENABLED > 0
#if MB_RTU_ENABLED > 0
        vMbsRTUTimerT35Expired(device1);
#endif
#if MB_ASCII_ENABLED > 0
        vMbsASCIITimerT1SExpired(device1);
#endif        
#endif

#if MB_MASTER_ENABLED > 0
#if MB_RTU_ENABLED > 0
        vMBMRTUTimerT35Expired(deviceM1);
#endif
#if MB_ASCII_ENABLED > 0
        vMBMASCIITimerT1SExpired(deviceM1);
#endif        

#endif
    }
}


#if MB_MASTER_ENABLED > 0

/*This optional function returns the current time in milliseconds (don't care
  for wraparound, this is only used for time diffs).*/
uint32_t xMBsys_now(void)
{
    return sysclocktime;
}
#endif

#endif

