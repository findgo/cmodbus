
#include "stm32f10x.h"
#include "systick.h"
//for driver
#include "modbus.h"

#include "logdrv.h"
#include <stdio.h>

/* Private define for reg modify by user ------------------------------------------------------------*/
#define REG_HOLDING_NREGS     ( 20 )
#define REG_INPUT_NREGS       ( 20 )
#define REG_COILS_SIZE        (8 * 3)
#define REG_DISCRETE_SIZE     (8 * 3)

static void prvnvicInit(void);

#if MB_MASTER_ENABLED > 0
Mbmhandle_t deviceM0;
Mbmhandle_t deviceM1;

static void nodeReqResultCB(MbReqResult_t result, MbException_t eException, void *req) {
    mo_logln(DEBUG, "error count: %d, result: %d", result, ((MbmReq_t *) req)->errcnt);
    //printf("error count: %d, result: %d\r\n" ,result,((MbmReq_t *)req)->errcnt);
}

int main(void) {
    MbmNode_t *node;
    MbErrorCode_t status;

//	prvClockInit();
    prvnvicInit();
    Systick_Configuration();
    SystemCoreClockUpdate();
    logInit();
//    Systick_Configuration();
//#if MB_RTU_ENABLED > 0
//    deviceM0 = MbmNew(MB_RTU, MBCOM0, 115200, MB_PAR_NONE);
//#elif MB_ASCII_ENABLED > 0
//    deviceM0= MbmNew(MB_ASCII, MBCOM0, 115200, MB_PAR_NONE);
//#endif
//    if (deviceM0) {
//        node = MbmNodeNew(0x01, 0, REG_HOLDING_NREGS, 0, REG_INPUT_NREGS,
//                          0, REG_COILS_SIZE, 0, REG_DISCRETE_SIZE);
//        status = MbmAddNode(deviceM0, node);
//        if (status == MB_ENOERR) {
//            (void) MbmReqRdHoldingRegister(deviceM0, 0x01, 0, REG_HOLDING_NREGS, 1000);
//            (void) MbmReqRdInputRegister(deviceM0, 0x01, 0, REG_INPUT_NREGS, 1000);
//            (void) MbmReqRdCoils(deviceM0, 0x01, 0, REG_COILS_SIZE, 1000);
//            (void) MbmReqRdDiscreteInputs(deviceM0, 0x01, 0, REG_DISCRETE_SIZE, 1000);
//        }
//        (void) MbmStart(deviceM0);
//    }
#if MB_RTU_ENABLED > 0
    deviceM1 = MbmNew(MB_RTU, MBCOM1, 115200, MB_PAR_NONE);
#elif MB_ASCII_ENABLED > 0
    deviceM1= MbmNew(MB_ASCII, MBCOM1, 115200, MB_PAR_NONE);
#endif
    if (deviceM1) {
        node = MbmNodeNew(0x01, 0, REG_HOLDING_NREGS, 0, REG_INPUT_NREGS,
                          0, REG_COILS_SIZE, 0, REG_DISCRETE_SIZE);
        if (node == NULL)
            return 0;

        MbmNodeCallBackAssign(node, nodeReqResultCB, NULL);
        status = MbmAddNode(deviceM1, node);
        if (status == MB_ENOERR) {
            (void) MbmReqRdHoldingRegister(deviceM1, 0x01, 0, REG_HOLDING_NREGS, 1000);
//           (void)MbmReqRdInputRegister(deviceM1, 0x01, 0, REG_INPUT_NREGS, 1000);        
//           (void)MbmReqRdCoils(deviceM1, 0x01, 0, REG_COILS_SIZE, 1000);        
//           (void)MbmReqRdDiscreteInputs(deviceM1, 0x01, 0, REG_DISCRETE_SIZE, 1000);        
        }
        (void) MbmStart(deviceM1);
    }

    mblogln("modbus master start!");
    while (1) {
        MbmPoll();
    }
    //Should never reach this point!
}

#endif


#if MB_SLAVE_ENABLED > 0

Mbshandle_t device0;
Mbshandle_t device1;
static __aligned(2) uint8_t dev0regbuf[REG_HOLDING_NREGS * 2 + REG_INPUT_NREGS * 2 + REG_COILS_SIZE / 8 + REG_DISCRETE_SIZE / 8] =
    {0xaa,0xaa,0xbb,0xbb,0xcc,0xcc,0xdd,0xdd,0xee,0xee,0xff,0xff,0xaa,0x55,0xaa,0xcc,0xff};
static __aligned(2) uint16_t dev1HoldingBuf[REG_HOLDING_NREGS] = {0x1111,0x2222,0x3333};
static __aligned(2) uint16_t dev1InputBuf[REG_INPUT_NREGS] = {0x4444,0x5555,0x6666};
static  uint8_t dev1CoilsBuf[ REG_COILS_SIZE / 8 ] = {0xaa,0x55};
static  uint8_t dev1DiscreteBuf[REG_DISCRETE_SIZE / 8] = {0x55,0xaa,0x77};

int main(void)
{	
    MbErrorCode_t status;

    prvnvicInit();
    Systick_Configuration();
//#if MB_RTU_ENABLED > 0
//    device0 = MbsNew(MB_RTU, 0x01, MBCOM0, 115200, MB_PAR_NONE);
//#elif MB_ASCII_ENABLED > 0
//    device0 = MbsNew(MB_ASCII, 0x01, MBCOM0, 115200, MB_PAR_NONE);
//#endif
//    if(device0){
//       status = MbsRegAssign(device0,
//                        dev0regbuf,
//                        sizeof(dev0regbuf),
//                        0,REG_HOLDING_NREGS ,
//                        0,REG_INPUT_NREGS,
//                        0,REG_COILS_SIZE,
//                        0,REG_DISCRETE_SIZE);
//       if(status == MB_ENOERR)
//            (void)MbsStart(device0);
//    }
#if MB_RTU_ENABLED > 0
    device1 = MbsNew(MB_RTU, 0x01, MBCOM1, 115200, MB_PAR_NONE);
#elif MB_ASCII_ENABLED > 0
    device1 = MbsNew(MB_ASCII, 0x01, MBCOM1, 115200, MB_PAR_NONE);
#endif
    if(device1){
       status = MbsRegAssignSingle(device1,
                        dev1HoldingBuf,
                        0,REG_HOLDING_NREGS ,
                        dev1InputBuf,
                        0,REG_INPUT_NREGS,
                        dev1CoilsBuf,
                        0,REG_COILS_SIZE,
                        dev1DiscreteBuf,
                        0,REG_DISCRETE_SIZE);
       if(status == MB_ENOERR)
            (void)MbsStart(device1);
    }
    while(1)
    {
        MbsPoll();
    }
    //Should never reach this point!
}
#endif

//nvic configuration
static void prvnvicInit(void) {
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
}


