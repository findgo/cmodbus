 一般情况下只需修改mbcpu.h 实现临界保护 和配置mbconfig.h进行裁剪
 实现portserial.c porttimer.c 
 极少数情况需要修改mbsrtu.c 或mbsascii.h 中的发送
 在使用中,包含头文件modbus.h即可使用

// example
```
Mbshandle_t device0;
Mbshandle_t device1;
static __align(2) uint8_t dev0regbuf[REG_HOLDING_NREGS * 2 + REG_INPUT_NREGS * 2 + REG_COILS_SIZE / 8 + REG_DISCRETE_SIZE / 8] = 
    {0xaa,0xaa,0xbb,0xbb,0xcc,0xcc,0xdd,0xdd,0xee,0xee,0xff,0xff,0xaa,0x55,0xaa,0xcc,0xff};
static __align(2) uint16_t dev1HoldingBuf[REG_HOLDING_NREGS] = {0x1111,0x2222,0x3333};
static __align(2) uint16_t dev1InputBuf[REG_INPUT_NREGS] = {0x4444,0x5555,0x6666};
static  uint8_t dev1CoilsBuf[ REG_COILS_SIZE / 8 ] = {0xaa,0x55};
static  uint8_t dev1DiscreteBuf[REG_DISCRETE_SIZE / 8] = {0x55,0xaa,0x77};

int main(void)
{	
    MbErrorCode_t status;

	prvClockInit();
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
```