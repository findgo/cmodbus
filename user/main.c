#include "app_cfg.h"
#include "systick.h"
//for driver
#include "modbus.h"
#include "mbfunc.h"

static void prvClockInit(void);
static void prvnvicInit(void);

#if MB_MASTER_ENABLE > 0
mb_MasterDevice_t* deviceM1;
uint8_t flag = 0;
uint16_t RegAddr,val[3] = {0x1111,0x2222,0x3333};
int main(void)
{	
    mb_ErrorCode_t status;
    mb_slavenode_t *node;
  
//	prvClockInit();
	  prvnvicInit();
    Systick_Configuration();
    SystemCoreClockUpdate();
    
	//Systick_Configuration();
#if MB_RTU_ENABLED > 0   
    deviceM1 = xMBMasterNew(MB_RTU, 0, 9600, MB_PAR_NONE);
    if(deviceM1){
       node = xMBMasterNodeNew(0x01,0,REG_HOLDING_NREGS ,0,REG_INPUT_NREGS,
                                    0,REG_COILS_SIZE,0,REG_DISCRETE_SIZE);
       if(node){
            eMBMasterNodeadd(deviceM1,node);
       }

       //(void)eMBReqRdHoldingRegister(deviceM1, 0x01, 0, REG_HOLDING_NREGS, 1000);
       //(void)eMBReqRdInputRegister(deviceM1, 0x01, 0, 3, 1000);        
       //(void)eMBReqRdCoils(deviceM1, 0x01, 0, 16, 1000);        
       //(void)eMBReqRdDiscreteInputs(deviceM1, 0x01, 0, 16, 1000);        
       (void)eMBMasterStart(deviceM1);  
       
    }
#endif
#if MB_ASCII_ENABLED > 0

#endif
	while(1)
	{
	    if(flag){
            flag = 0;
            eMBReqRdWrMulHoldingRegister(deviceM1, 0x01,0,REG_HOLDING_NREGS, RegAddr,3,val,3);
            //eMbReqWrMulHoldingRegister(deviceM1, 0x01, RegAddr,3,val,3);
            //eMBReqWrHoldingRegister(deviceM1, 0x01, RegAddr,val);
            //eMbReqWrMulCoils(deviceM1, 0x01, RegAddr,8,(uint8_t *)&val,1);
            //eMBReqWrCoil(deviceM1, 0x01, RegAddr, val,);
        }
	    vMBMasterPoll();
	}
	//Should never reach this point!
}


#endif

#if MB_SLAVE_ENABLE > 0

mb_Device_t device1;
static __align(2) uint8_t dev1regbuf[REG_HOLDING_NREGS * 2 + REG_INPUT_NREGS * 2 + REG_COILS_SIZE / 8 + REG_DISCRETE_SIZE / 8] = 
    {0xaa,0xaa,0xbb,0xbb,0xcc,0xcc,0xdd,0xdd,0xee,0xee,0xff,0xff,0xaa,0x55,0xaa,0xcc,0xff};
mb_Device_t device2;
int main(void)
{	
    mb_ErrorCode_t status;

	prvClockInit();
	prvnvicInit();
	//Systick_Configuration();
#if MB_RTU_ENABLED > 0   
    status = eMBOpen(&device1,MB_RTU, 0x01, 0, 9600, MB_PAR_NONE);
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
#endif
#if MB_ASCII_ENABLED > 0
    status = eMBOpen(&device1,MB_ASCII, 0x01, 0, 9600, MB_PAR_NONE);
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
#endif
	while(1)
	{
        vMBPoll();
	}
	//Should never reach this point!
}
#endif

/*
 *=============================================================================
 *						  System Clock Configuration
 *=============================================================================
 *		 System Clock source		  | PLL(HSI)
 *-----------------------------------------------------------------------------
 *		 SYSCLK 					  | 64000000 Hz
 *-----------------------------------------------------------------------------
 *		 HCLK					  | 64000000 Hz
 *-----------------------------------------------------------------------------
 *		 PCLK1					  | 32000000 Hz
 *-----------------------------------------------------------------------------
 *		 PCLK2					  | 64000000 Hz
 *-----------------------------------------------------------------------------
 *		 ADCCLK					  | 10670000 Hz
 *-----------------------------------------------------------------------------
 *		 AHB Prescaler			  | 1
 *-----------------------------------------------------------------------------
 *		 APB1 Prescaler 			  | 2
 *-----------------------------------------------------------------------------
 *		 APB2 Prescaler 			  | 1
 *-----------------------------------------------------------------------------
 *		 ADC Prescaler 			  | 6
 *-----------------------------------------------------------------------------
 *		 HSI Frequency			  | 8000000 Hz   /   2
 *-----------------------------------------------------------------------------
 *		 PLL MUL					  | 16
 *-----------------------------------------------------------------------------
 *		 VDD						  | 3.3 V
 *-----------------------------------------------------------------------------
 *		 Flash Latency			  | 2 WS
 *-----------------------------------------------------------------------------
 *=============================================================================
*/
//HSI Clock configuration
// 当不使用外部时钟时，配置内部时钟参数
static void prvClockInit(void)
{
    RCC_DeInit();
    /* Enable HSi */
    RCC_HSICmd(ENABLE);

    /* Enable Prefetch Buffer */
    FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);

    /* Flash 2 wait state */
    FLASH_SetLatency(FLASH_Latency_2);

    /* AHB = DIV1 , HCLK = SYSCLK */
    RCC_HCLKConfig(RCC_SYSCLK_Div1);

    /* APB2 = DIV1, PCLK2 = HCLK / 2 */
    RCC_PCLK2Config(RCC_HCLK_Div1);

    /* APB1 = DIV2, PCLK1 = HCLK */
    RCC_PCLK1Config(RCC_HCLK_Div2);

    /* ADCCLK = PCLK2/6 = 64 / 6 = 10.67 MHz*/
    RCC_ADCCLKConfig(RCC_PCLK2_Div4);

    /* PLLCLK = 8 / 2MHz * 16 = 64 MHz */
    RCC_PLLConfig(RCC_PLLSource_HSI_Div2, RCC_PLLMul_16);

    /* Enable PLL */
    RCC_PLLCmd(ENABLE);

    /* Wait till PLL is ready */
    while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);

    /* Select PLL as system clock source */
    RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);

    /* Wait till PLL is used as system clock source */
    while(RCC_GetSYSCLKSource() != 0x08); 
}

//nvic configuration
static void prvnvicInit(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
}




