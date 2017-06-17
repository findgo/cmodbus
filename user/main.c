#include "app_cfg.h"

//for driver
#include "modbus.h"

static void prvClockInit(void);
static void prvnvicInit(void);

mb_device_t device1;
static __align(2) uint8_t dev1regbuf[REG_HOLDING_NREGS * 2 + REG_INPUT_NREGS * 2 + REG_COILS_SIZE / 8 + REG_DISCRETE_SIZE / 8] = 
    {0xaa,0xaa,0xbb,0xbb,0xcc,0xcc,0xdd,0xdd,0xee,0xee,0xff,0xff,0xaa,0x55,0xaa};
mb_device_t device2;
int main(void)
{	
    eMBErrorCode status;

	prvClockInit();
	prvnvicInit();
	//Systick_Configuration();
     
    status = eMBOpen(&device1,0,MB_RTU, 0x01, 0, 9600, MB_PAR_NONE);
    if(status == MB_ENOERR){
       status = eMBRegCreate(&device1,
                        dev1regbuf,
                        0,
                        REG_HOLDING_NREGS ,
                        0,
                        REG_INPUT_NREGS,
                        0,
                        REG_COILS_SIZE,
                        0,
                        REG_DISCRETE_SIZE);
       if(status == MB_ENOERR)
            (void)eMBStart(&device1);
    }
//    status = eMBOpen(&device2,1,MB_RTU, 0x02, 0, 9600, MB_PAR_NONE);
//    if(status == MB_ENOERR){
//       status = eMBRegCreate(&device2,
//                        dev1regbuf,
//                        0,
//                        REG_HOLDING_NREGS ,
//                        0,
//                        REG_INPUT_NREGS,
//                        0,
//                        REG_COILS_SIZE,
//                        0,
//                        REG_DISCRETE_SIZE);
//       if(status == MB_ENOERR)
//            (void)eMBStart(&device2);
//    }
	while(1)
	{
        vMBPoll();
	}
	//Should never reach this point!
}

/*
 *=============================================================================
 *						  System Clock Configuration
 *=============================================================================
 *		 System Clock source		  | PLL(HSE)
 *-----------------------------------------------------------------------------
 *		 SYSCLK 					  | 72000000 Hz
 *-----------------------------------------------------------------------------
 *		 HCLK					  | 72000000 Hz
 *-----------------------------------------------------------------------------
 *		 PCLK1					  | 36000000 Hz
 *-----------------------------------------------------------------------------
 *		 PCLK2					  | 72000000 Hz
 *-----------------------------------------------------------------------------
 *		 ADCCLK					  | 12000000 Hz
 *-----------------------------------------------------------------------------
 *		 AHB Prescaler			  | 1
 *-----------------------------------------------------------------------------
 *		 APB1 Prescaler 			  | 2
 *-----------------------------------------------------------------------------
 *		 APB2 Prescaler 			  | 1
 *-----------------------------------------------------------------------------
 *		 ADC Prescaler 			  | 6
 *-----------------------------------------------------------------------------
 *		 HSE Frequency			  | 8000000 Hz
 *-----------------------------------------------------------------------------
 *		 PLL MUL					  | 9
 *-----------------------------------------------------------------------------
 *		 VDD						  | 3.3 V
 *-----------------------------------------------------------------------------
 *		 Flash Latency			  | 2 WS
 *-----------------------------------------------------------------------------
 *=============================================================================
*/
//Clock configuration
static void prvClockInit(void)
{
  ErrorStatus HSEStartUpStatus;

  RCC_DeInit();
  /* Enable HSE */
  RCC_HSEConfig(RCC_HSE_ON);
  /* Wait till HSE is ready */
  HSEStartUpStatus = RCC_WaitForHSEStartUp();

  if (HSEStartUpStatus == SUCCESS)
  {
    /* Enable Prefetch Buffer */
    FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);

    /* Flash 2 wait state */
    FLASH_SetLatency(FLASH_Latency_2);

    /* AHB = DIV1 , HCLK = SYSCLK */
    RCC_HCLKConfig(RCC_SYSCLK_Div1);

    /* APB2 = DIV1, PCLK2 = HCLK */
    RCC_PCLK2Config(RCC_HCLK_Div1);

    /* APB1 = DIV2, PCLK1 = HCLK/2 */
    RCC_PCLK1Config(RCC_HCLK_Div2);

    /* ADCCLK = PCLK2/6 = 72 / 6 = 12 MHz*/
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);

    /* PLLCLK = 8MHz * 9 = 72 MHz */
    RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_9);

    /* Enable PLL */
    RCC_PLLCmd(ENABLE);

    /* Wait till PLL is ready */
    while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);

    /* Select PLL as system clock source */
    RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);

    /* Wait till PLL is used as system clock source */
    while(RCC_GetSYSCLKSource() != 0x08);

	SystemCoreClockUpdate();
  } else {
    //Cannot start xtal oscillator!
    while(1); 
  }
}

//nvic configuration
static void prvnvicInit(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
}




