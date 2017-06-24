
#ifndef __MODBUS_H_
#define __MODBUS_H_

#include "mb.h"
#include "mbfunc.h"


/* for bits */
/* for request */
mb_ErrorCode_t eMBReqRdCoils(mb_MasterDevice_t *Mdev, uint8_t slaveaddr, 
                                        uint16_t RegStartAddr, uint16_t Coilcnt, uint16_t scanrate);
mb_ErrorCode_t eMBReqWrCoil(mb_MasterDevice_t *Mdev, uint8_t slaveaddr, 
                                        uint16_t RegAddr, uint16_t val);
mb_ErrorCode_t eMbReqWrMulCoils(mb_MasterDevice_t *Mdev, uint8_t slaveaddr, 
                                        uint16_t RegStartAddr, uint16_t Coilcnt,
                                        uint8_t *valbuf, uint16_t valcnt);
mb_ErrorCode_t eMBReqRdDiscreteInputs(mb_MasterDevice_t *Mdev, uint8_t slaveaddr, 
                                        uint16_t RegStartAddr, uint16_t Discnt, uint16_t scanrate);

/* for register */                                    
/* for request */
mb_ErrorCode_t eMBReqRdHoldingRegister(mb_MasterDevice_t *Mdev, uint8_t slaveaddr, 
                                                uint16_t RegStartAddr, uint16_t Regcnt, uint16_t scanrate);
mb_ErrorCode_t eMBReqWrHoldingRegister(mb_MasterDevice_t *Mdev, uint8_t slaveaddr, 
                                                uint16_t RegAddr, uint16_t val);
mb_ErrorCode_t eMbReqWrMulHoldingRegister(mb_MasterDevice_t *Mdev, uint8_t slaveaddr, 
                                                uint16_t RegStartAddr, uint16_t Regcnt,
                                                uint16_t *valbuf, uint16_t valcnt);
mb_ErrorCode_t eMBReqRdInputRegister(mb_MasterDevice_t *Mdev, uint8_t slaveaddr, 
                                                uint16_t RegStartAddr, uint16_t Regcnt, uint16_t scanrate);

mb_ErrorCode_t eMBReqRdWrMulHoldingRegister(mb_MasterDevice_t *Mdev, uint8_t slaveaddr, 
                                                uint16_t RegReadStartAddr, uint16_t RegReadCnt,
                                                uint16_t RegWriteStartAddr, uint16_t RegWriteCnt,
                                                uint16_t *valbuf, uint16_t valcnt);




#endif

